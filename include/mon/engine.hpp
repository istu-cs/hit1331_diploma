#pragma once

#include <mon/poller.hpp>

#include <cppdb/backend.h>
#include <cppdb/pool.h>

#include <boost/noncopyable.hpp>

#include <memory>

namespace mon {

class engine : private boost::noncopyable {
public:
    engine(const std::shared_ptr<mon::poller> &poller,
           const cppdb::pool::pointer &db_pool);

    cppdb::pool::pointer db() { return m_db_pool; }

    void add_agent(const AgentConfiguration &agent);
    void remove_agent(const std::string &agent_id);

    void handle_response(const CheckResponse &response);

private:
    void init_db();
    void init_poller();
    void init_agent(const std::string &agent_id);

    const std::shared_ptr<mon::poller> m_poller;
    const cppdb::pool::pointer m_db_pool;
};

}  // namespace mon
