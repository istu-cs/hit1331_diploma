#include <mon/engine.hpp>

#include <bunsan/logging/trivial.hpp>

#include <cppdb/frontend.h>
#include <cppdb/backend.h>

namespace mon {

engine::engine(const std::shared_ptr<mon::poller> &poller,
               const cppdb::pool::pointer &db_pool):
        m_poller(poller),
        m_db_pool(db_pool) {
    init_db();
    init_poller();
}

void engine::init_db() {
    cppdb::session sql(m_db_pool->open());
    sql << "CREATE TABLE IF NOT EXISTS agents ("
           "    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
           "    name TEXT NOT NULL,"
           "    target TEXT NOT NULL"
           ")" << cppdb::exec;
    sql << "CREATE TABLE IF NOT EXISTS results ("
           "    agent_id INTEGER NOT NULL REFERENCES agents,"
           "    query TEXT NOT NULL,"
           "    status INTEGER NOT NULL,"
           "    message TEXT NOT NULL,"
           "    CONSTRAINT results_pk PRIMARY KEY (agent_id, query)"
           ")" << cppdb::exec;
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
        m_poller->add_agent(agent);
        init_agent(agent.id());
    }
}

void engine::init_agent(const std::string &agent_id) {
    CheckRequest query;
    query.set_interval(30);

    init_result(agent_id, "cpu");
    query.set_id("cpu_" + agent_id);
    query.set_agent(agent_id);
    query.mutable_plugin()->set_id("cpu");
    query.mutable_plugin()->clear_argument();
    m_poller->add_query(query);

    init_result(agent_id, "memory");
    query.set_id("memory_" + agent_id);
    query.set_agent(agent_id);
    query.mutable_plugin()->set_id("memory");
    query.mutable_plugin()->clear_argument();
    m_poller->add_query(query);

    init_result(agent_id, "disk");
    query.set_id("disk_" + agent_id);
    query.set_agent(agent_id);
    query.mutable_plugin()->set_id("disk");
    query.mutable_plugin()->clear_argument();
    m_poller->add_query(query);

    init_result(agent_id, "firebird");
    query.set_id("firebird_" + agent_id);
    query.set_agent(agent_id);
    query.mutable_plugin()->set_id("process");
    query.mutable_plugin()->clear_argument();
    query.mutable_plugin()->add_argument("firebird");
    m_poller->add_query(query);
}

void engine::init_result(const std::string &agent_id, const std::string &query) {
    cppdb::session sql(m_db_pool->open());
    sql << "INSERT OR REPLACE INTO "
           "results (agent_id, query, status, message) "
           "VALUES(?, ?, ?, ?)"
        << agent_id
        << query
        << CheckResponse::UNKNOWN
        << "N/A"
        << cppdb::exec;
}

void engine::add_agent(const AgentConfiguration &agent) {
    m_poller->add_agent(agent);
    init_agent(agent.id());
}

void engine::remove_agent(const std::string &agent_id) {
    cppdb::session sql(m_db_pool->open());
    sql << "DELETE FROM agents "
           "WHERE id = ?"
        << agent_id
        << cppdb::exec;
    sql << "DELETE FROM results "
           "WHERE agent_id = ?"
        << agent_id
        << cppdb::exec;
    m_poller->remove_agent(agent_id);
    m_poller->remove_query("cpu_" + agent_id);
    m_poller->remove_query("memory_" + agent_id);
    m_poller->remove_query("disk_" + agent_id);
    m_poller->remove_query("firebird_" + agent_id);
}

void engine::handle_response(const CheckResponse &response) {
    BUNSAN_LOG_DEBUG << "Response " << response.ShortDebugString();
    cppdb::session sql(m_db_pool->open());
    sql << "INSERT OR REPLACE INTO "
           "results (agent_id, query, status, message) "
           "VALUES(?, ?, ?, ?)"
        << response.request().agent()
        << response.request().id()
        << response.status()
        << response.message()
        << cppdb::exec;
}

}  // namespace mon

