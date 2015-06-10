#pragma once

#include <mon/poller.hpp>

#include <cppcms/application.h>

#include <memory>

namespace mon {
namespace web {

class monitor : public cppcms::application
{
public:
    monitor(cppcms::service &srv,
            const std::shared_ptr<mon::poller> &poller);

    void main(std::string url) override;

private:
    void add();
    void edit(std::string agent_id);
    void remove(std::string agent_id);
    void show();

    const std::shared_ptr<mon::poller> m_poller;
};

}  // namespace web
}  // namespace mon
