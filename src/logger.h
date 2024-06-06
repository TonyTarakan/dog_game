#ifndef GAME_SERVER_LOGGER_H
#define GAME_SERVER_LOGGER_H

#include <string>
#include <string_view>
#include <chrono>

#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

#include <boost/json.hpp>
#include <boost/date_time.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", boost::json::value);
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime);
BOOST_LOG_ATTRIBUTE_KEYWORD(msg, "Msg", std::string_view);

namespace logger {

namespace logging = boost::log;
namespace json = boost::json;
using namespace std::literals;

void JsonFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);

class Logger {
public:
    static Logger& get_instance() {
        static Logger instance;
        return instance;
    }

    static void log_json(std::string_view message, const json::value& json_value) {
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, json_value) << message;
    }

private:
    Logger() {
        logging::add_common_attributes();
        logging::add_console_log(
            std::cout,
            logging::keywords::format = &JsonFormatter,
            logging::keywords::auto_flush = true
        );
    }
};

}

#endif //GAME_SERVER_LOGGER_H
