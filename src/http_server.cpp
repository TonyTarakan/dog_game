#include "http_server.h"
#include <boost/asio/dispatch.hpp>
#include <iostream>

namespace http_server {

void ReportError(beast::error_code ec, std::string_view info) {
    using namespace std::literals;
    logger::Logger::log_json("error", {{"code", 1}, {"text", ec.what()}, {"where", info}});
}

void SessionBase::Run() {
    net::dispatch(stream_.get_executor(), beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}

void SessionBase::Read() {
    using namespace std::literals;
    // Очищаем запрос от прежнего значения (метод Read может быть вызван несколько раз)
    request_ = {};
    stream_.expires_after(30s);
    // Считываем request_ из stream_, используя buffer_ для хранения считанных данных
    http::async_read(stream_, buffer_, request_, beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
}

void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
    using namespace std::literals;
    if (ec == http::error::end_of_stream) {
        return Close();
    }
    if (ec) {
        return ReportError(ec, "read"sv);
    }
    logger::Logger::log_json("request received",{
        {"ip", stream_.socket().remote_endpoint().address().to_string()},
        {"URI", request_.target()},
        {"method", request_.method_string()},
    });
    begin_ = std::chrono::steady_clock::now();
    HandleRequest(std::move(request_));
}

void SessionBase::Close() {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    // TODO: проверить код ошибки ec и зарефакторить прекоды
}


}  // namespace http_server
