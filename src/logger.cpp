#include "logger.h"

void logger::JsonFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    auto message = rec[logging::expressions::smessage];
    auto ts = rec[timestamp];
    auto data = rec[additional_data];

    json::value jv = {
        {"timestamp", to_iso_extended_string(*ts)},
        {"data", data.get()},
        {"message", message->c_str()}
    };

    strm << jv;
}