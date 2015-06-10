#pragma once

#include <mon/web/content/base.hpp>

#include <string>
#include <vector>

namespace mon {
namespace web {
namespace content {

struct remove : base {
    struct agent {
        std::string name;
        std::string target;
    };
    std::vector<agent> agents;
};

}  // namespace content
}  // namespace web
}  // namespace mon
