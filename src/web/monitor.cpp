#include <mon/web/monitor.hpp>

#include <mon/web/content/edit.hpp>
#include <mon/web/content/error.hpp>
#include <mon/web/content/show.hpp>

#include <cppcms/url_dispatcher.h>
#include <cppcms/url_mapper.h>

namespace mon {
namespace web {

monitor::monitor(cppcms::service &srv,
                 const std::shared_ptr<mon::poller> &poller):
        cppcms::application(srv),
        m_poller(poller) {
    dispatcher().assign("/edit", &monitor::edit, this);
    mapper().assign("edit", "/edit");

    dispatcher().assign("/show", &monitor::show, this);
    mapper().assign("show", "/show");

    mapper().root("/monitor");
}

void monitor::edit() {
    content::edit data;
    render("edit", data);
}

void monitor::show() {
    content::show data;
    render("show", data);
}

void monitor::main(std::string url) {
    if (!dispatcher().dispatch(url))
    {
        response().status(cppcms::http::response::not_found);
        content::error error;
        error.brief = translate("Page not found.");
        error.message = translate("Requested page was not found. Try to use menu above.");
        render("error", error);
    }
}

}  // namespace web
}  // namespace mon
