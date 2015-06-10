#pragma once

#include <cppcms/form.h>

namespace mon {
namespace web {
namespace content {
namespace form {

struct agent : cppcms::form {
    agent() {
        target.name("target");
        target.message(cppcms::locale::translate("Target"));
        add(target);
        name.name("name");
        name.message(cppcms::locale::translate("Name"));
        add(name);
        group.name("group");
        group.message(cppcms::locale::translate("Group"));
        add(group);
        submit.value(cppcms::locale::translate("Submit"));
        add(submit);
    }

    cppcms::widgets::text target;
    cppcms::widgets::text name;
    cppcms::widgets::text group;
    cppcms::widgets::submit submit;
};

}  // namespace form
}  // namespace content
}  // namespace web
}  // namespace mon
