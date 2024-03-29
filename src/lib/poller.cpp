#include <mon/poller.hpp>

#include <grpc++/async_unary_call.h>
#include <grpc++/channel_arguments.h>
#include <grpc++/create_channel.h>

#include <bunsan/logging/trivial.hpp>

namespace mon {

poller::poller(boost::asio::io_service &io_service):
        m_io_service(io_service),
        m_agent_worker(io_service),
        m_query_worker(io_service) {
    m_agent_worker.post(std::bind(&poller::work, this));
}

poller::~poller() {
    if (m_alive) close();
}

void poller::close() {
    m_agent_worker.dispatch([this] {
        m_alive = false;
    });
}

void poller::add_agent(const AgentConfiguration &agent_) {
    const auto new_agent = std::make_shared<agent>();
    new_agent->configuration = agent_;
    m_agent_worker.post([this, new_agent] {
        m_agents[new_agent->configuration.id()] = new_agent;
        BUNSAN_LOG_INFO << "Added agent id = " << new_agent->configuration.id();
    });
}

void poller::remove_agent(const std::string &agent_id) {
    const std::string id = agent_id;
    m_agent_worker.post([this, id] {
        m_agents.erase(id);
        BUNSAN_LOG_INFO << "Removed agent id = " << id;
    });
}

void poller::add_query(const CheckRequest &query_) {
    const CheckRequest configuration = query_;
    m_agent_worker.post([this, configuration] {
        query q;
        q.configuration = configuration;
        q.next_call = std::chrono::system_clock::now();
        const std::string id = q.configuration.agent() + "#" + q.configuration.query();
        m_queries[id] = q;
        BUNSAN_LOG_INFO << "Added query id = " << id;
    });
}

void poller::remove_query(const std::string &agent_id,
                          const std::string &query) {
    const auto id = agent_id + "#" + query;
    m_agent_worker.post([this, id] {
        m_queries.erase(id);
    });
    BUNSAN_LOG_INFO << "Removed query id = " << id;
}

void poller::work() {
    // return early if possible
    if (!m_alive) return;

    for (auto &ag_ : m_agents) {
        agent &ag = *ag_.second;
        BUNSAN_LOG_DEBUG << "Found agent id = " << ag.configuration.id();
        if (!ag.channel) {
            BUNSAN_LOG_INFO << "Connection to agent at "
                            << ag.configuration.connection().target();
            ag.channel = grpc::CreateChannel(ag.configuration.connection().target(),
                                             grpc::InsecureCredentials(),
                                             grpc::ChannelArguments());
            ag.stub.reset();
        }
        if (!ag.stub) {
            ag.stub = Agent::NewStub(ag.channel);
        }
    }
    const auto now = std::chrono::system_clock::now();
    for (auto &q_ : m_queries) {
        query &q = q_.second;
        const auto ag = m_agents.find(q.configuration.agent());
        if (ag == m_agents.end()) continue;  // skip unmatched queries
        if (q.next_call > now) continue;  // too early
        const std::chrono::system_clock::duration interval =
            std::chrono::seconds(q.configuration.interval());
        q.next_call += interval;
        if (q.next_call + interval < now) {
            // force forward if outdated
            q.next_call = now + interval;
        }
        q.agent_ref = ag->second;
        BUNSAN_LOG_DEBUG << "Posted query id = " << q.configuration.query()
                         << " for agent id = " << q.configuration.agent();
        m_query_worker.post(std::bind(&poller::perform_query, this, q));
    }

    // continue if alive, must be last call
    if (m_alive) {
        // note that we schedule work after all queries
        m_query_worker.post([this] {
            m_agent_worker.post(std::bind(&poller::work, this));
        });
    }
}

void poller::perform_query(const query &q) {
    grpc::ClientContext context;
    context.set_deadline(
        // FIXME hardcoded timeout
        std::chrono::system_clock::now() + std::chrono::seconds(10)
    );
    CheckResponse response;
    const grpc::Status status = q.agent_ref->stub->Check(
        &context,
        q.configuration,
        &response
    );
    *response.mutable_request() = q.configuration;
    switch (status.code()) {
    case grpc::OK:
        // no modifications required
        break;
    case grpc::DEADLINE_EXCEEDED:
        response.set_status(CheckResponse::RPC_TIMEOUT);
        response.set_message(status.details());
        break;
    default:
        response.set_status(CheckResponse::RPC_ERROR);
        response.set_message(status.details());
    }
    m_io_service.post([this, response] {
        try {
            m_check_response_signal(response);
        } catch (std::exception &e) {
            BUNSAN_LOG_ERROR << "on_check() report for response "
                             << response.ShortDebugString()
                             << " got error: " << e.what();
        }
    });
}

}  // namespace mon
