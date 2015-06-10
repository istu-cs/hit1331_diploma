#include <mon/web/monitor.hpp>

#include <mon/web/content/edit.hpp>
#include <mon/web/content/error.hpp>
#include <mon/web/content/remove.hpp>
#include <mon/web/content/show.hpp>

#include <cppcms/url_dispatcher.h>
#include <cppcms/url_mapper.h>

namespace mon {
namespace web {

monitor::monitor(cppcms::service &srv,
                 const std::shared_ptr<mon::poller> &poller):
        cppcms::application(srv),
        m_poller(poller) {
    dispatcher().assign("/add", &monitor::add, this);
    mapper().assign("add", "/add");

    dispatcher().assign("/edit/(\\d+)", &monitor::edit, this, 1);
    mapper().assign("edit", "/edit/{1}");

    dispatcher().assign("/remove/(\\d+)", &monitor::remove, this, 1);
    mapper().assign("remove", "/remove/{1}");

    dispatcher().assign("/show", &monitor::show, this);
    mapper().assign("show", "/show");

    mapper().root("/monitor");
}

void monitor::add() {
    content::edit data;
    if (request().request_method() == "POST") {
        data.agent.load(context());
        if (data.agent.validate()) {
            // TODO submit to DB
        }
        response().set_redirect_header(url("show"));
    } else {
        render("edit", data);
    }
}

void monitor::edit(const std::string agent_id) {
    content::edit data;
    if (request().request_method() == "POST") {
        data.agent.load(context());
        if (data.agent.validate()) {
            // TODO submit to DB
        }
        response().set_redirect_header(url("show"));
    } else {
        // TODO load data from DB
        render("edit", data);
    }
}

void monitor::remove(const std::string agent_id) {
    content::remove data;
    render("remove", data);
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
