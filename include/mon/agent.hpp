#pragma once

#include <mon/server.grpc.pb.h>

namespace mon {

class agent_service final : public Agent::Service {
public:
    agent_service() = default;

    grpc::Status Check(grpc::ServerContext *context,
                       const CheckRequest *request,
                       CheckResponse *response) override;
};

}  // namespace mon
