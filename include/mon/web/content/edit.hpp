#pragma once

#include <mon/web/content/base_form.hpp>
#include <mon/web/content/form/agent.hpp>

#include <boost/optional.hpp>

#include <string>

namespace mon {
namespace web {
namespace content {

struct edit : base_form {
    form::agent agent;
};

}  // namespace content
}  // namespace web
}  // namespace mon
