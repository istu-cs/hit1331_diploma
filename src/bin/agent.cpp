#include <mon/agent.hpp>

#include <bunsan/application.hpp>
#include <bunsan/logging/trivial.hpp>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_credentials.h>

using namespace bunsan::application;

namespace
{
    class agent_application: public application
    {
    public:
        using application::application;

        void initialize_argument_parser(argument_parser &parser) override
        {
            application::initialize_argument_parser(parser);
            parser.add_options()
            (
                "listen",
                value<std::string>(&listen)->default_value("0.0.0.0:31999"),
                "Listen on ip:port, e.g. 0.0.0.0:31999 (default)"
            );
        }

        int main(const variables_map &/*variables*/) override
        {
            mon::agent_service agent;
            grpc::ServerBuilder builder;
            builder.AddListeningPort(listen, grpc::InsecureServerCredentials());
            builder.RegisterService(&agent);
            auto server = builder.BuildAndStart();
            BUNSAN_LOG_INFO << "Listening on " << listen;
            server->Wait();
            return 0;
        }

    private:
        std::string listen;
    };
}

int main(int argc, char **argv)
{
    agent_application app(argc, argv);
    app.name("mon::agent");
    return app.exec();
}
