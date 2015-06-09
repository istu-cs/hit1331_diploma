#include <mon/poller.hpp>

#include <mon/server.grpc.pb.h>

#include <grpc++/async_unary_call.h>
#include <grpc++/channel_arguments.h>
#include <grpc++/create_channel.h>

namespace mon {

struct check_context {
    CheckResponse response;
    grpc::Status status;
};

poller::poller(boost::asio::io_service &io_service):
        m_io_service(io_service) {
    post_listen();
}

void poller::listen() {
    void *tag;
    bool ok;
    if (!m_queue.Next(&tag, &ok)) return;
    if (ok) {
        std::unique_ptr<check_context> ctx(reinterpret_cast<check_context *>(tag));
        if (ctx->status.IsOk()) {
            const CheckResponse response = ctx->response;
            m_io_service.dispatch([this, response] {
                m_check_response_signal(response);
            });
        }
    }
    post_listen();
}

void poller::post_listen() {
    m_io_service.post(std::bind(&poller::listen, this));
}

void poller::poll(const AgentConfiguration &agent) {
    const std::string agent_id = agent.id();
    const std::string target = agent.connection().target();
    for (const Query &query : agent.query()) {
        const Plugin plugin = query.plugin();
        const std::uint64_t interval = query.interval();
        {
            std::lock_guard<std::mutex> lk(m_lock);
            if (m_channels.find(target) == m_channels.end()) {
                m_channels[target] =
                    grpc::CreateChannel(agent.connection().target(),
                                        grpc::InsecureCredentials(),
                                        grpc::ChannelArguments());
            }
        }
        m_io_service.dispatch([this, agent_id, plugin, target] {
            std::lock_guard<std::mutex> lk(m_lock);
            const auto iter = m_channels.find(target);
            if (iter == m_channels.end()) {
                m_io_service.dispatch([this] {
                    CheckResponse response;
                    response.set_status(CheckResponse::CANCELLED);
                    m_check_response_signal(response);
                });
            } else {
                std::shared_ptr<grpc::ChannelInterface> channel = iter->second;
                m_io_service.dispatch([this, agent_id, channel, plugin] {
                    CheckRequest request;
                    request.set_agent(agent_id);
                    *request.mutable_plugin() = plugin;
                    grpc::ClientContext context;
                    std::unique_ptr<Agent::Stub> stub(Agent::NewStub(channel));
                    std::unique_ptr<grpc::ClientAsyncResponseReader<CheckResponse>> rpc(
                        stub->AsyncCheck(&context, request, &m_queue)
                    );
                    auto ctx = std::make_unique<check_context>();
                    auto ctx_ = ctx.release();
                    rpc->Finish(&ctx_->response, &ctx_->status, ctx_);
                });
            }
        });
    }
}

void poller::remove_poll(const AgentConfiguration &agent) {
    std::lock_guard<std::mutex> lk(m_lock);
    m_channels.erase(agent.connection().target());
}

}  // namespace mon
