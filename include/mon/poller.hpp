#pragma once

#include <mon/server.pb.h>
#include <mon/server.grpc.pb.h>

#include <grpc++/channel_interface.h>

#include <boost/asio/io_service.hpp>
#include <boost/asio/strand.hpp>
#include <boost/noncopyable.hpp>
#include <boost/signals2/signal.hpp>

#include <memory>
#include <unordered_map>

namespace mon {

class poller : private boost::noncopyable {
public:
    using connection = boost::signals2::connection;
    using check_response_signal = boost::signals2::signal<void(const CheckResponse &)>;

    explicit poller(boost::asio::io_service &io_service);
    ~poller();

    void close();

    void add_agent(const AgentConfiguration &agent);
    void remove_agent(const std::string &agent_id);

    void add_query(const CheckRequest &query);
    void remove_query(const std::string &agent_id,
                      const std::string &query);

    connection on_check(const check_response_signal::slot_type &slot) {
        return m_check_response_signal.connect(slot);
    }
    connection on_check_extended(const check_response_signal::extended_slot_type &slot) {
        return m_check_response_signal.connect_extended(slot);
    }

private:
    struct agent {
        AgentConfiguration configuration;
        std::shared_ptr<grpc::ChannelInterface> channel;
        std::unique_ptr<Agent::Stub> stub;
    };

    struct query {
        CheckRequest configuration;
        std::chrono::system_clock::time_point next_call;
        std::shared_ptr<agent> agent_ref;
    };

    void work();
    void perform_query(const query &q);

    boost::asio::io_service &m_io_service;
    check_response_signal m_check_response_signal;
    boost::asio::io_service::strand m_agent_worker;
    boost::asio::io_service::strand m_query_worker;
    bool m_alive = true;
    std::unordered_map<std::string, std::shared_ptr<agent>> m_agents;
    std::unordered_map<std::string, query> m_queries;
};

}  // namespace mon
