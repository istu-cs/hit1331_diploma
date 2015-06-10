#pragma once

#include <mon/poller.hpp>

#include <cppcms/application.h>
#include <cppdb/backend.h>
#include <cppdb/pool.h>

#include <memory>

namespace mon {
namespace web {

class monitor : public cppcms::application
{
public:
    monitor(cppcms::service &srv,
            const std::shared_ptr<mon::poller> &poller,
            const cppdb::pool::pointer &db_pool);

    void main(std::string url) override;

private:
    void init_db();
    void add();
    void edit(std::string agent_id);
    void remove();
    void show();

    const std::shared_ptr<mon::poller> m_poller;
    const cppdb::pool::pointer m_db_pool;
};

}  // namespace web
}  // namespace mon
