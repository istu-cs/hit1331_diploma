#pragma once

#include <mon/server.pb.h>

#include <grpc++/channel_interface.h>

#include <boost/asio/io_service.hpp>
#include <boost/noncopyable.hpp>
#include <boost/signals2/signal.hpp>

#include <mutex>
#include <unordered_map>

namespace mon {

class poller : private boost::noncopyable {
public:
    using connection = boost::signals2::connection;
    using check_response_signal = boost::signals2::signal<void (const CheckResponse &)>;

    explicit poller(boost::asio::io_service &io_service);

    void poll(const AgentConfiguration &agent);
    void remove_poll(const AgentConfiguration &agent);
    connection on_check(const check_response_signal::slot_type &slot) {
        return m_check_response_signal.connect(slot);
    }
    connection on_check_extended(const check_response_signal::extended_slot_type &slot) {
        return m_check_response_signal.connect_extended(slot);
    }

private:
    void post_listen();
    void listen();

    boost::asio::io_service &m_io_service;
    check_response_signal m_check_response_signal;
    std::mutex m_lock;
    std::unordered_map<std::string, std::shared_ptr<grpc::ChannelInterface>> m_channels;
    grpc::CompletionQueue m_queue;
};

}  // namespace mon
