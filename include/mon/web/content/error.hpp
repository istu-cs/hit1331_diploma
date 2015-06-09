#pragma once

#include <mon/web/content/base.hpp>

namespace mon {
namespace web {
namespace content {

struct error : base {
    std::string brief;
    std::string message;
    bool raw = false; // Should it be rendered as raw html?
};

}  // namespace content
}  // namespace web
}  // namespace mon
