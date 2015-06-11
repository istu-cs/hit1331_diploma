#include <mon/web/monitor.hpp>

#include <mon/web/content/edit.hpp>
#include <mon/web/content/error.hpp>
#include <mon/web/content/remove.hpp>
#include <mon/web/content/show.hpp>

#include <cppcms/url_dispatcher.h>
#include <cppcms/url_mapper.h>
#include <cppdb/frontend.h>
#include <cppdb/backend.h>

#include <map>

namespace mon {
namespace web {

monitor::monitor(cppcms::service &srv,
                 const std::shared_ptr<mon::poller> &poller,
                 const cppdb::pool::pointer &db_pool):
        cppcms::application(srv),
        m_poller(poller),
        m_db_pool(db_pool) {
    init_db();

    dispatcher().assign("/add", &monitor::add, this);
    mapper().assign("add", "/add");

    dispatcher().assign("/edit/(\\d+)", &monitor::edit, this, 1);
    mapper().assign("edit", "/edit/{1}");

    dispatcher().assign("/remove", &monitor::remove, this);
    mapper().assign("remove", "/remove");

    dispatcher().assign("/show", &monitor::show, this);
    mapper().assign("show", "/show");

    mapper().root("/monitor");
}

void monitor::init_db() {
    cppdb::session sql(m_db_pool->open());
    sql << "CREATE TABLE IF NOT EXISTS agents ("
           "    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
           "    name TEXT NOT NULL,"
           "    target TEXT NOT NULL"
           ")" << cppdb::exec;
    sql << "CREATE TABLE IF NOT EXISTS results ("
           "    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
           "    agent_id INTEGER NOT NULL REFERENCES agents,"
           "    query TEXT NOT NULL,"
           "    status INTEGER NOT NULL,"
           "    message TEXT NOT NULL"
           ")" << cppdb::exec;
}

void monitor::add() {
    cppdb::session sql(m_db_pool->open());
    content::edit data;
    if (request().request_method() == "POST") {
        data.agent.load(context());
        if (data.agent.validate()) {
            sql << "INSERT INTO agents (name, target)"
                   "VALUES(?, ?)"
                << data.agent.name.value()
                << data.agent.target.value()
                << cppdb::exec;
        }
        response().set_redirect_header(url("show"));
    } else {
        render("edit", data);
    }
}

void monitor::edit(const std::string agent_id) {
    cppdb::session sql(m_db_pool->open());
    content::edit data;
    if (request().request_method() == "POST") {
        data.agent.load(context());
        if (data.agent.validate()) {
            sql << "UPDATE agents "
                   "SET name = ?, "
                   "target = ? "
                   "WHERE id = ?"
                << data.agent.name.value()
                << data.agent.target.value()
                << agent_id
                << cppdb::exec;
        }
        response().set_redirect_header(url("show"));
    } else {
        cppdb::result result =
            sql << "SELECT name, target "
                   "FROM agents "
                   "WHERE id = ?"
                << agent_id
                << cppdb::row;
        if (result.empty()) {
            response().status(cppcms::http::response::not_found);
            content::error error;
            error.brief = translate("Agent not found.");
            error.message = translate("Requested agent was not found.");
            render("error", error);
        }
        data.agent.name.value(result.get<std::string>("name"));
        data.agent.target.value(result.get<std::string>("target"));
        render("edit", data);
    }
}

void monitor::remove() {
    cppdb::session sql(m_db_pool->open());
    if (request().request_method() == "POST") {
        content::remove data;
        render("remove", data);
    } else {
        response().status(cppcms::http::response::method_not_allowed);
        content::error error;
        error.brief = translate("Page not found.");
        error.message = translate("POST is required!");
        render("error", error);
    }
}

void monitor::show() {
    cppdb::session sql(m_db_pool->open());
    content::show data;
    data.plugins = {
        { "cpu", "cpu load" },
        { "memory", "memory load" },
        { "disk", "disk load" },
        { "firebird", "firebird presence" },
    };
    std::unordered_map<std::string, std::size_t> query2id;
    for (std::size_t i = 0; i < data.plugins.size(); ++i) {
        query2id[data.plugins[i].id] = i;
    }
    std::map<std::int64_t, content::show::agent> agents;
    cppdb::result result =
        sql << "SELECT id, name, target "
               "FROM agents";
    while (result.next()) {
        std::int64_t id;
        std::string name;
        std::string target;
        result >> id >> name >> target;
        agents[id].name = name;
        agents[id].target = target;
        agents[id].stats.resize(data.plugins.size());
    }
    result =
        sql << "SELECT id, agent_id, query, status, message "
               "FROM results";
    while (result.next()) {
        std::int64_t id;
        std::int64_t agent_id;
        std::string query;
        std::int64_t status;
        std::string message;
        result >> id >> agent_id >> query >> status >> message;
        auto ag = agents.find(agent_id);
        if (ag == agents.end()) continue;  // skip unknown agent
        const auto query_id = query2id.find(query);
        if (query_id == query2id.end()) continue;  // skip unknown query
        ag->second.stats[query_id->second].set_status(CheckResponse::Status(status));
        ag->second.stats[query_id->second].set_message(message);
    }
    render("show", data);
}

void monitor::main(std::string url) {
    if (!dispatcher().dispatch(url)) {
        response().status(cppcms::http::response::not_found);
        content::error error;
        error.brief = translate("Page not found.");
        error.message = translate("Requested page was not found. Try to use menu above.");
        render("error", error);
    }
}

}  // namespace web
}  // namespace mon
