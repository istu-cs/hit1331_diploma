#include <mon/engine.hpp>

#include <bunsan/logging/trivial.hpp>

#include <cppdb/frontend.h>
#include <cppdb/backend.h>

namespace mon {

engine::engine(const std::shared_ptr<mon::poller> &poller,
               const cppdb::pool::pointer &db_pool,
               const bool initialize):
        m_poller(poller),
        m_db_pool(db_pool) {
    if (initialize) init_db();
    init_poller();
}

void engine::init_db() {
    cppdb::session sql(m_db_pool->open());
    try { sql << "DROP TABLE results" << cppdb::exec; } catch (cppdb::cppdb_error &) {}
    try { sql << "DROP TABLE agents" << cppdb::exec; } catch (cppdb::cppdb_error &) {}
    try {
        sql << "DROP FUNCTION upsert_result(integer,text,integer,text)" << cppdb::exec;
    } catch (cppdb::cppdb_error &) {}
    sql << "CREATE TABLE agents ("
           "    id SERIAL PRIMARY KEY,"
           "    name TEXT NOT NULL,"
           "    target TEXT NOT NULL"
           ")" << cppdb::exec;
    sql << "CREATE TABLE results ("
           "    agent_id INTEGER NOT NULL REFERENCES agents,"
           "    query TEXT NOT NULL,"
           "    status INTEGER NOT NULL,"
           "    message TEXT NOT NULL,"
           "    CONSTRAINT results_pk PRIMARY KEY (agent_id, query)"
           ")" << cppdb::exec;
    sql << "CREATE OR REPLACE FUNCTION upsert_result(agent_id_ INTEGER, "
           "                                         query_ TEXT, "
           "                                         status_ INTEGER, "
           "                                         message_ TEXT) RETURNS void as $$\n"
           "BEGIN\n"
           "    UPDATE results set status = status_, message = message_ "
           "        WHERE agent_id = agent_id_ AND query = query_;\n"
           "    IF FOUND THEN\n"
           "        RETURN;\n"
           "    END IF;\n"
           "    BEGIN\n"
           "        INSERT INTO results (agent_id, query, status, message) "
           "            VALUES (agent_id_, query_, status_, message_);\n"
           "    EXCEPTION WHEN OTHERS THEN\n"
           "        UPDATE results set status = status_, message = message_ "
           "            WHERE agent_id = agent_id_ AND query = query_;\n"
           "    END;\n"
           "    RETURN;\n"
           "END;\n"
           "$$ language plpgsql;"
        << cppdb::exec;
}

void engine::init_poller() {
    cppdb::session sql(m_db_pool->open());
    cppdb::result result =
        sql << "SELECT id, target "
               "FROM agents";
    AgentConfiguration agent;
    while (result.next()) {
        result >> *agent.mutable_id()
               >> *agent.mutable_connection()->mutable_target();
        init_agent(agent.id());
        m_poller->add_agent(agent);
    }
}

void engine::init_agent(const std::string &agent_id) {
    CheckRequest query;
    query.set_interval(30);

    init_result(agent_id, "cpu");
    query.set_agent(agent_id);
    query.set_query("cpu");
    query.mutable_plugin()->set_id("cpu");
    query.mutable_plugin()->clear_argument();
    m_poller->add_query(query);

    init_result(agent_id, "memory");
    query.set_agent(agent_id);
    query.set_query("memory");
    query.mutable_plugin()->set_id("memory");
    query.mutable_plugin()->clear_argument();
    m_poller->add_query(query);

    init_result(agent_id, "disk");
    query.set_agent(agent_id);
    query.set_query("disk");
    query.mutable_plugin()->set_id("disk");
    query.mutable_plugin()->clear_argument();
    m_poller->add_query(query);

    init_result(agent_id, "firebird");
    query.set_agent(agent_id);
    query.set_query("firebird");
    query.mutable_plugin()->set_id("process");
    query.mutable_plugin()->clear_argument();
    query.mutable_plugin()->add_argument("firebird");
    m_poller->add_query(query);
}

void engine::init_result(const std::string &agent_id, const std::string &query) {
    CheckResponse response;
    response.mutable_request()->set_query(query);
    response.mutable_request()->set_agent(agent_id);
    response.set_status(CheckResponse::UNKNOWN);
    response.set_message("N/A");
    set_result(response);
}

void engine::set_result(const CheckResponse &response) {
    cppdb::session sql(m_db_pool->open());
    cppdb::result stat =
        sql << "SELECT upsert_result(?, ?, ?, ?)"
            << response.request().agent()
            << response.request().query()
            << response.status()
            << response.message()
            << cppdb::row;
}

void engine::add_agent(const AgentConfiguration &agent) {
    m_poller->remove_agent(agent.id());
    m_poller->add_agent(agent);
    init_agent(agent.id());
}

void engine::remove_agent(const std::string &agent_id) {
    cppdb::session sql(m_db_pool->open());
    sql << "DELETE FROM results "
           "WHERE agent_id = ?"
        << agent_id
        << cppdb::exec;
    sql << "DELETE FROM agents "
           "WHERE id = ?"
        << agent_id
        << cppdb::exec;
    m_poller->remove_agent(agent_id);
    m_poller->remove_query(agent_id, "cpu");
    m_poller->remove_query(agent_id, "memory");
    m_poller->remove_query(agent_id, "disk");
    m_poller->remove_query(agent_id, "firebird");
}

void engine::handle_response(const CheckResponse &response) {
    BUNSAN_LOG_DEBUG << "Response " << response.ShortDebugString();
    set_result(response);
}

}  // namespace mon
