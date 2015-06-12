#include <mon/engine.hpp>
#include <mon/poller.hpp>
#include <mon/web/monitor.hpp>

#include <bunsan/application.hpp>
#include <bunsan/filesystem/fstream.hpp>
#include <bunsan/logging/trivial.hpp>
#include <bunsan/shared_cast.hpp>

#include <cppcms/applications_pool.h>
#include <cppcms/service.h>
#include <cppdb/pool.h>

#include <boost/asio/io_service.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/scope_exit.hpp>
#include <boost/thread.hpp>

#include <iostream>
#include <functional>
#include <memory>

#include <cstdlib>

using namespace bunsan::application;

namespace {
struct json_error: virtual bunsan::error {
    using error_line = boost::error_info<struct tag_error_line, int>;
};

class agent_application: public application {
public:
    using application::application;

    void initialize_argument_parser(argument_parser &parser) override {
        application::initialize_argument_parser(parser);
        parser.add_options()
        (
            "c,configuration",
            value<std::string>(&configuration)->required(),
            "JSON configuration file"
        )
        (
            "initialize",
            bool_switch(&initialize),
            "Initialize environment (database, etc)"
        );
    }

    int main(const variables_map &/*variables*/) override {
        cppcms::service srv(load_configuration());
        const cppdb::pool::pointer db_pool = cppdb::pool::create(
            srv.settings().get<std::string>("mon.db")
        );
        boost::asio::io_service io_service;
        boost::thread_group threads;
        BOOST_SCOPE_EXIT_ALL(&) {
            io_service.stop();
            threads.join_all();
        };
        auto work = std::make_unique<boost::asio::io_service::work>(io_service);
        threads.create_thread([&io_service] { io_service.run(); });
        threads.create_thread([&io_service] { io_service.run(); });
        threads.create_thread([&io_service] { io_service.run(); });
        threads.create_thread([&io_service] { io_service.run(); });
        const auto poller = std::make_shared<mon::poller>(io_service);
        const auto engine = std::make_shared<mon::engine>(poller, db_pool, initialize);
        const auto bengine = bunsan::shared_cast(engine);
        poller->on_check(mon::poller::check_response_signal::slot_type(
            &mon::engine::handle_response, engine.get(), _1
        ).track(bengine));
        srv.applications_pool().mount(
            cppcms::applications_factory<mon::web::monitor>(engine)
        );
        srv.run();
        work.reset();
        return 0;
    }

private:
    cppcms::json::value load_configuration() {
        bunsan::filesystem::ifstream fin(configuration);
        cppcms::json::value config;
        int error_line;
        if (!config.load(fin, true, &error_line)) {
            BOOST_THROW_EXCEPTION(
                json_error() <<
                json_error::error_line(error_line));
        }
        fin.close();
        return config;
    }

    std::string configuration;
    bool initialize;
};
}  // namespace

int main(int argc, char **argv) {
    agent_application app(argc, argv);
    app.name("mon::server");
    return app.exec();
}
