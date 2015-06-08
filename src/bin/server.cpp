#include <bunsan/application.hpp>
#include <bunsan/logging/trivial.hpp>

using namespace bunsan::application;

namespace
{
    class server_application: public application
    {
    public:
        using application::application;

        void initialize_argument_parser(argument_parser &parser) override
        {
            application::initialize_argument_parser(parser);
        }

        int main(const variables_map &variables) override
        {
            return 0;
        }
    };
}

int main(int argc, char **argv)
{
    server_application app(argc, argv);
    app.name("mon::server");
    return app.exec();
}
