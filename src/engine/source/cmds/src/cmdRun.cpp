#include "cmds/cmdRun.hpp"

#include <atomic>
#include <csignal>
#include <thread>
#include <vector>

#include <hlp/hlp.hpp>
#include <kvdb/kvdbManager.hpp>
#include <logging/logging.hpp>
#include <rxbk/rxFactory.hpp>

#include "base/utils/getExceptionStack.hpp"
#include "builder.hpp"
#include "catalog.hpp"
#include "engineServer.hpp"
#include "protocolHandler.hpp"
#include "register.hpp"

namespace
{
constexpr auto WAIT_DEQUEUE_TIMEOUT_USEC = 1 * 1000000;

// variables for handling threads
std::atomic<bool> gs_doRun = true;
std::vector<std::thread> gs_threadList;

void sigint_handler(const int signum)
{
    // Inform threads that they must exit
    gs_doRun = false;

    for (auto& t : gs_threadList)
    {
        t.join();
    };

    // TODO: this should not be necessary, but server threads are not terminating.
    exit(0);
}
} // namespace

namespace cmd
{
void run(const std::string& kvdbPath,
         const std::string& endpoint,
         const int queueSize,
         const int threads,
         const std::string& fileStorage,
         const std::string& environment,
         const int logLevel)
{

    // Set Crt+C handler
    sigset_t sig_empty_mask;
    sigemptyset(&sig_empty_mask);

    struct sigaction sigintAction;
    sigintAction.sa_handler = sigint_handler;
    sigintAction.sa_mask = sig_empty_mask;

    sigaction(SIGINT, &sigintAction, NULL);

    // Init logging
    logging::LoggingConfig logConfig;
    switch (logLevel)
    {
        case 0: logConfig.logLevel = logging::LogLevel::Debug; break;
        case 1: logConfig.logLevel = logging::LogLevel::Info; break;
        case 2: logConfig.logLevel = logging::LogLevel::Warn; break;
        case 3: logConfig.logLevel = logging::LogLevel::Error; break;
        default: WAZUH_LOG_ERROR("Invalid log level: {}", logLevel);
    }
    logging::loggingInit(logConfig);

    KVDBManager::init(kvdbPath);

    engineserver::EngineServer server {{endpoint}, static_cast<size_t>(queueSize)};
    if (!server.isConfigured())
    {
        WAZUH_LOG_ERROR("Could not configure server for endpoint [{}], engine "
                        "inizialization aborted.",
                        endpoint);
        return;
    }

    catalog::Catalog _catalog(catalog::StorageType::Local, fileStorage);

    auto hlpParsers =
        _catalog.getFileContents(catalog::AssetType::Schema, "wazuh-logql-types");
    // TODO because builders don't have access to the catalog we are configuring
    // the parser mappings on start up for now
    hlp::configureParserMappings(hlpParsers);

    try
    {
        builder::internals::registerBuilders();
    }
    catch (const std::exception& e)
    {
        WAZUH_LOG_ERROR("Exception while registering builders: [{}]",
                        utils::getExceptionStack(e));
        return;
    }
    // TODO: Handle errors on construction
    builder::Builder<catalog::Catalog> _builder(_catalog);
    decltype(_builder.buildEnvironment(environment)) env;
    try
    {
        env = _builder.buildEnvironment(environment);
    }
    catch (const std::exception& e)
    {
        WAZUH_LOG_ERROR("Exception while building environment: [{}]",
                        utils::getExceptionStack(e));
        return;
    }

    // Processing Workers (Router), Router is replicated in each thread
    // TODO: handle hot modification of routes
    for (auto i = 0; i < threads; ++i)
    {
        std::thread t {
            [=, &eventBuffer = server.output()]()
            {
                auto controller = rxbk::buildRxPipeline(env);

                // Thread loop
                while (gs_doRun)
                {
                    std::string event;

                    if (eventBuffer.wait_dequeue_timed(event, WAIT_DEQUEUE_TIMEOUT_USEC))
                    {
                        try
                        {
                            auto result = base::result::makeSuccess(
                                engineserver::ProtocolHandler::parse(event));
                            controller.ingestEvent(
                                std::make_shared<base::result::Result<base::Event>>(
                                    std::move(result)));
                        }
                        catch (const std::exception& e)
                        {
                            WAZUH_LOG_ERROR(
                                "An error ocurred while parsing a message: [{}]",
                                e.what());
                        }
                    }
                }

                controller.complete();
                return 0;
            }};

        gs_threadList.push_back(std::move(t));
    }

    server.run();

    logging::loggingTerm();
}
} // namespace cmd