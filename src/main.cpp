/* TODO
 * - Improve architecture using clang-uml
 * - Add more unit tests
 * - Add benchmarks and load tests
 * - Group pytest tests
 * - Optimize for performance (strand for each session, etc.)
 * - Change from boost json to nlohmann
 * - Change from boost serialization to protobuf
 * - Change from boost beast to crow or another web framework
 * - Try something for geometry
 * - Improve logging, maybe replace boost log
 * - Add the Rust version of the game
 */


#include <filesystem>
#include <iostream>
#include <thread>

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <boost/signals2.hpp>

#include "infrastructure.h"
#include "json_loader.h"
#include "logger.h"
#include "request_handler.h"
#include "sdk.h"
#include "ticker.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sig = boost::signals2;
namespace sys = boost::system;
namespace bop = boost::program_options;
namespace fs = std::filesystem;

namespace {

template <typename FnT>
void RunWorkers(unsigned thread_count, const FnT& work_function) {
    thread_count = std::max(1u, thread_count);
    std::vector<std::jthread> workers;
    workers.reserve(thread_count - 1);
    // Запускаем thread_count-1 рабочих потоков, выполняющих функцию work_function
    while (--thread_count) {
        workers.emplace_back(work_function);
    }
    work_function();
}

}  // namespace

struct Args {
    unsigned int tick_period = 0;
    std::string config_path;
    std::string www_path;
    bool random_spawn = false;
    std::string state_path;
    unsigned int autosave_period = 0;
};

[[nodiscard]] std::optional<Args> ParseArgs(int argc, const char* const argv[]) {
    Args args;
    bop::options_description opts_desc("Allowed options");
    opts_desc.add_options()
        ("help,h", "produce help message")
        ("tick-period,t", bop::value<unsigned>(&args.tick_period)->value_name("milliseconds"), "tick period")
        ("config-file,c", bop::value<std::string>(&args.config_path)->value_name("file"), "config file path")
        ("www-root,w", bop::value<std::string>(&args.www_path)->value_name("dir"), "static files root")
        ("randomize-spawn-points", "spawn dogs at random positions ")
        ("state-file", bop::value<std::string>(&args.state_path)->value_name("file"), "state save/restore file path")
        ("save-state-period", bop::value<unsigned>()->value_name("milliseconds"), "autosave period");

    bop::variables_map vm;
    bop::store(bop::parse_command_line(argc, argv, opts_desc), vm);
    bop::notify(vm);

    if (vm.count("help") || vm.empty()) {
        std::cout << opts_desc << std::endl;
        return std::nullopt;
    }
    if (vm.count("config-file")) {
        args.config_path = vm["config-file"].as<std::string>();
    }
    if (vm.count("www-root")) {
        args.www_path = vm["www-root"].as<std::string>();
    }
    if (vm.count("tick-period")) {
        args.tick_period = vm["tick-period"].as<unsigned int>();
    }
    if (vm.count("randomize-spawn-points")) {
        args.random_spawn = true;
    }

    if (vm.count("state-file")) {
        args.state_path = vm["state-file"].as<std::string>();
    }
    if (vm.count("save-state-period")) {
        args.autosave_period = vm["save-state-period"].as<unsigned int>();
    }

    return std::optional<Args>{std::move(args)};
}

int main(int argc, const char* argv[]) {
    Args args;

    auto arg_opt = ParseArgs(argc, argv);
    if (arg_opt == std::nullopt) {
        return EXIT_FAILURE;
    }
    args = *arg_opt;
    logger::Logger::get_instance();

    try {
        auto game = json_loader::LoadGame(args.config_path);
        game->SetRandomSpawn(args.random_spawn);

        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(static_cast<int>(num_threads));

        // Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
            }
        });

        // Создаём обработчик HTTP-запросов и связываем его с моделью игры
        fs::path static_content_path{fs::weakly_canonical(args.www_path)};

        auto api_global_strand = net::make_strand(ioc);

        if (args.tick_period > 0) {
            auto ticker = std::make_shared<Ticker>(
                    api_global_strand,
                    milliseconds(args.tick_period),
                    [&game](milliseconds delta) { game->ExternalTick(delta); }
            );
            ticker->Start();
        }

        // TODO: arg
//        std::string db_url = std::getenv("GAME_DB_URL");
        std::string db_url =  "postgres://postgres:postgres@127.0.0.1:5432";
        db::RecordDB db{db_url};

        app::App app{game, db};
        infrastructure::Autosaver autosaver{app, args.state_path, milliseconds{args.autosave_period}};
        if (args.autosave_period > 0) {
            autosaver.Restore();
        }

        sig::scoped_connection conn1 = app.GetGame()->DoOnTick(
            [&autosaver, &app](milliseconds delta) mutable {
                autosaver.OnTick(delta);
                app.RetireDogs();
            }
        );

        http_handler::RequestHandler handler{api_global_strand, app, static_content_path};

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080; // TODO: add arg
        http_server::ServeHttp(
            ioc,
            {address, port},
            [&handler](auto&& req, auto&& send) {
                handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
            });

        logger::Logger::log_json("server started", {{"port", port}, {"address", address.to_string()}});

        RunWorkers(num_threads, [&ioc] {
            ioc.run();
        });

        if (args.autosave_period > 0) {
            autosaver.Save();
        }

        logger::Logger::log_json("server exited", {{"code", 0}});

    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        logger::Logger::log_json("server exited", {{"code", 1}, {"error", ex.what()}});
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
