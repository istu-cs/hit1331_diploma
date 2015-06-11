#pragma once

#include <mon/engine.hpp>
#include <mon/poller.hpp>

#include <cppcms/application.h>

#include <memory>

namespace mon {
namespace web {

class monitor : public cppcms::application {
public:
    monitor(cppcms::service &srv,
            const std::shared_ptr<mon::engine> &engine);

    void main(std::string url) override;

private:
    void add();
    void edit(std::string agent_id);
    void remove();
    void show();

    const std::shared_ptr<mon::engine> m_engine;
};

}  // namespace web
}  // namespace mon
