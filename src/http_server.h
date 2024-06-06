#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "sdk.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <chrono>

#include "logger.h"

namespace http_server {

namespace net = boost::asio;
namespace beast = boost::beast;
namespace sys = boost::system;
namespace http = beast::http;

using tcp = net::ip::tcp;
using HttpRequest = http::request<http::string_body>;

using namespace std::chrono;

void ReportError(beast::error_code ec, std::string_view what);

class SessionBase {
public:
    SessionBase(const SessionBase&) = delete;
    SessionBase& operator=(const SessionBase&) = delete;
    void Run();

protected:
    explicit SessionBase(tcp::socket&& socket) : stream_(std::move(socket)) {}
    ~SessionBase() = default;

    template <typename Body, typename Fields>
    void Write(http::response<Body, Fields>&& response) {
        auto safe_response = std::make_shared<http::response<Body, Fields>>(std::move(response));
        auto self = GetSharedThis();
        http::async_write(stream_, *safe_response,
                          [safe_response, self](beast::error_code ec, std::size_t bytes_written) {
                              self->OnWrite<Body, Fields>(safe_response, ec, bytes_written);
                          });
    }

    template <typename Body, typename Fields>
    void OnWrite(std::shared_ptr<http::response<Body, Fields>> response,
                 beast::error_code ec,
                 [[maybe_unused]] std::size_t bytes_written) {
        using namespace std::literals;

        logger::Logger::log_json("response sent",{
            {"ip", stream_.socket().remote_endpoint().address().to_string()},
            {"response_time", duration_cast<milliseconds>(steady_clock::now() - begin_).count()},
            {"code", static_cast<unsigned>(http::status_class(response->result()))},
            {"content_type", response->base()["content-type"]},
        });

        if (ec) {
            return ReportError(ec, "write"sv);
        }
        if (response->need_eof()) {
            return Close();
        }
        Read();
    }

private:
    void Read();
    void OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read);
    void Close();
    virtual void HandleRequest(HttpRequest&& request) = 0;
    virtual std::shared_ptr<SessionBase> GetSharedThis() = 0;

private:
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    HttpRequest request_;
    std::chrono::steady_clock::time_point begin_;
};


template <typename RequestHandlerT>
class Session : public SessionBase, public std::enable_shared_from_this<Session<RequestHandlerT>> {
public:
    template <typename Handler>
    Session(tcp::socket&& socket, Handler&& request_handler)
            : SessionBase(std::move(socket))
            , request_handler_(std::forward<Handler>(request_handler)) {
    }

private:
    RequestHandlerT request_handler_;
    std::shared_ptr<SessionBase> GetSharedThis() override {
        return this->shared_from_this();
    }
    void HandleRequest(HttpRequest&& request) override {
        request_handler_(std::move(request),
                         [self = this->shared_from_this()](auto&& response) {
                             self->Write(std::forward<decltype(response)>(response));
                         }
        );
    }
};

template <typename RequestHandlerT>
class Listener : public std::enable_shared_from_this<Listener<RequestHandlerT>> {
public:
    template <typename Handler>
    Listener(net::io_context& ioc, const tcp::endpoint& endpoint, Handler&& request_handler)
            : ioc_(ioc)
            , acceptor_(net::make_strand(ioc))
            , request_handler_(std::forward<Handler>(request_handler)) {

        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(net::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen(net::socket_base::max_listen_connections);
    }

    void Run() {
        DoAccept();
    }

private:

    void DoAccept() {
        acceptor_.async_accept(net::make_strand(ioc_),
                               beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this()));
    }

    void OnAccept(sys::error_code ec, tcp::socket socket) {
        using namespace std::literals;
        if (ec) {
            return ReportError(ec, "accept"sv);
        }
        AsyncRunSession(std::move(socket));
        DoAccept();
    }

    void AsyncRunSession(tcp::socket&& socket) {
        std::make_shared<Session<RequestHandlerT>>(std::move(socket), request_handler_)->Run();
    }

private:
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    RequestHandlerT request_handler_;
};

template <typename RequestHandlerT>
void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandlerT&& handler) {
    using MyListener = Listener<std::decay_t<RequestHandlerT>>;
    std::make_shared<MyListener>(ioc, endpoint, std::forward<RequestHandlerT>(handler))->Run();
}

}  // namespace http_server

#endif  // HTTP_SERVER_H