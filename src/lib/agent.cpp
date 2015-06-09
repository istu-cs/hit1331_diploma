#include <mon/agent.hpp>

#include <bunsan/logging/trivial.hpp>

namespace mon {

grpc::Status agent_service::Check(grpc::ServerContext * /*context*/,
                                  const CheckRequest *request,
                                  CheckResponse *response) {
    *response->mutable_request() = *request;
    // TODO this is stub
    BUNSAN_LOG_DEBUG << request->ShortDebugString();
    response->set_status(CheckResponse::OK);
    response->set_message("Working fine!");
    return grpc::Status::OK;
}

}  // namespace mon
