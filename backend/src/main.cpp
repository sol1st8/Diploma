#include "application.h"
#include "http_server/http_server.h"
#include "request_handler.h"
#include "sync/thread_loader.h"
#include "config/dynamic_config.h"

#include <boost/asio/signal_set.hpp>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <memory>

namespace net = boost::asio;
namespace sys = boost::system;
using tcp = net::ip::tcp;
using namespace std::literals;

namespace {

template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::thread> workers;
    workers.reserve(n - 1);
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();

    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

} // namespace

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: run_server <static-data-path>"sv
                  << std::endl;
        return EXIT_FAILURE;
    }
    try {
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
            }
        });

        const std::filesystem::path home_path(std::filesystem::current_path().parent_path());
        const std::filesystem::path static_path(std::filesystem::weakly_canonical(home_path / argv[1]));

        http_handler::RequestHandler handler{static_path};

        http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
            handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });

        const std::filesystem::path config_path = home_path / "application.conf";
        try {
            std::cout << "Loading dynamic config from: " << config_path << std::endl;
            
            config::g_config.LoadFromFile(config_path.string());
            
            config::g_config.StartFileWatcher(config_path.string(), 10);
            
            std::cout << "Dynamic configuration loaded (version: " 
                     << config::g_config.GetVersion() << ")" << std::endl;
            std::cout << "File watcher started (checking every 10 seconds)" << std::endl;
            
            auto cfg = config::g_config.Get();
            std::cout << "Configuration parameters:" << std::endl;
            std::cout << "  Sync enabled: " << (cfg->sync_enabled ? "true" : "false") << std::endl;
            std::cout << "  Sync interval: " << cfg->sync_interval_seconds << " seconds" << std::endl;
            std::cout << "  Central DB: " << cfg->central_host << ":" << cfg->central_port 
                      << "/" << cfg->central_database << std::endl;
            std::cout << "  Regional DB: " << cfg->regional_host << ":" << cfg->regional_port 
                      << "/" << cfg->regional_database << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Warning: Failed to load dynamic config: " << e.what() << std::endl;
            std::cerr << "Using default configuration" << std::endl;
        }

        std::unique_ptr<sync_load::ThreadLoader> sync_loader;
        try {
            auto sync_config = config::g_config.GetSyncConfig();
            
            if (sync_config.sync_interval_seconds > 0) {
                sync_loader = std::make_unique<sync_load::ThreadLoader>(sync_config);
                sync_loader->Start();
                
                std::cout << "Database synchronization started (interval: "
                          << sync_config.sync_interval_seconds << " seconds)" << std::endl;
                std::cout << "  Central server ID: " << sync_config.central_server_id << std::endl;
                std::cout << "  Regional server ID: " << sync_config.regional_server_id << std::endl;
            } 
            else {
                std::cout << "Database synchronization is disabled" << std::endl;
            }
        } 
        catch (const std::exception& e) {
            std::cerr << "Warning: Failed to initialize sync: " << e.what() << std::endl;
            std::cerr << "Server will continue without synchronization" << std::endl;
        }

        std::cout << "Server has started..."sv << std::endl;

        RunWorkers(num_threads, [&ioc] {
            ioc.run();
        });
        
        std::cout << "Stopping configuration file watcher..." << std::endl;
        config::g_config.StopFileWatcher();
        
        if (sync_loader) {
            std::cout << "Stopping database synchronization..." << std::endl;
            sync_loader->Stop();
        }
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
