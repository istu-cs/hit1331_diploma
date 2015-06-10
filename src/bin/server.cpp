#include <mon/poller.hpp>
#include <mon/web/monitor.hpp>

#include <cppcms/applications_pool.h>
#include <cppcms/service.h>
#include <cppdb/pool.h>

#include <boost/asio/io_service.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/thread.hpp>

#include <iostream>
#include <functional>
#include <memory>

#include <cstdlib>

int main(int argc, char *argv[])
{
    try
    {
        cppcms::service srv(argc, argv);
        const cppdb::pool::pointer db_pool = cppdb::pool::create(
            srv.settings().get<std::string>("mon.db")
        );
        boost::asio::io_service io_service;
        boost::thread_group threads;
        auto work = std::make_unique<boost::asio::io_service::work>(io_service);
        threads.create_thread([&io_service] { io_service.run(); });
        threads.create_thread([&io_service] { io_service.run(); });
        threads.create_thread([&io_service] { io_service.run(); });
        threads.create_thread([&io_service] { io_service.run(); });
        const std::shared_ptr<mon::poller> poller = std::make_shared<mon::poller>(io_service);
        srv.applications_pool().mount(
            cppcms::applications_factory<mon::web::monitor>(poller, db_pool)
        );
        srv.run();
        work.reset();
        poller->close();
        threads.join_all();
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
