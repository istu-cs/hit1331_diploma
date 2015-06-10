#pragma once

#include <mon/server.pb.h>
#include <mon/web/content/base_form.hpp>

#include <string>
#include <vector>
#include <unordered_map>

namespace mon {
namespace web {
namespace content {

struct show : base_form {
    struct agent {
        std::string name;
        std::string target;
        std::unordered_map<std::string, CheckResponse> stats;
    };
    struct plugin {
        std::string id;
        std::string name;
    };
    std::vector<agent> agents;
    std::vector<plugin> plugins;
};

}  // namespace content
}  // namespace web
}  // namespace mon
