#include <mon/poller.hpp>

#include <mon/server.grpc.pb.h>

#include <grpc++/async_unary_call.h>
#include <grpc++/channel_arguments.h>
#include <grpc++/create_channel.h>

namespace mon {

namespace {
struct check_context {
    grpc::ClientContext rpc_context;
    CheckRequest request;
    CheckResponse response;
    grpc::Status status;
};
}  // namespace

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
        } else {
            CheckResponse response;
            *response.mutable_request() = ctx->request;
            switch (ctx->status.code()) {
            case grpc::DEADLINE_EXCEEDED:
                response.set_status(CheckResponse::RPC_TIMEOUT);
                break;
            default:
                response.set_status(CheckResponse::RPC_ERROR);
            }
            response.set_message(ctx->status.details());
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
                    response.set_status(CheckResponse::USER_CANCELLED);
                    m_check_response_signal(response);
                });
            } else {
                std::shared_ptr<grpc::ChannelInterface> channel = iter->second;
                m_io_service.dispatch([this, agent_id, channel, plugin] {
                    auto ctx = std::make_unique<check_context>();
                    ctx->request.set_agent(agent_id);
                    *ctx->request.mutable_plugin() = plugin;
                    ctx->rpc_context.set_deadline(
                        // FIXME hardcoded timeout
                        std::chrono::system_clock::now() + std::chrono::minutes(5)
                    );
                    std::unique_ptr<Agent::Stub> stub(Agent::NewStub(channel));
                    std::unique_ptr<grpc::ClientAsyncResponseReader<CheckResponse>> rpc(
                        stub->AsyncCheck(&ctx->rpc_context, ctx->request, &m_queue)
                    );
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
