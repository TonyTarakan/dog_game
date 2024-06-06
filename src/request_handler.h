#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <filesystem>
#include <optional>

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/json.hpp>
#include <boost/system.hpp>

#include "app.h"
#include "handler_serializer.h"
#include "http_server.h"
#include "model.h"
#include "model_json.h"

namespace http_handler {

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
namespace sys = boost::system;
namespace fs = std::filesystem;
using namespace std::literals;
using Strand = net::strand<net::io_context::executor_type>;
using StrReqt = http::request<http::string_body>;
using StrResp = http::response<http::string_body>;

struct ContentType {
    ContentType() = delete;
    using CON_TYPE = std::string_view;
    constexpr static CON_TYPE TEXT_HTML = "text/html"sv;
    constexpr static CON_TYPE TEXT_CSS = "text/css"sv;
    constexpr static CON_TYPE TEXT_PLAIN = "text/plain"sv;
    constexpr static CON_TYPE TEXT_JS = "text/javascript"sv;
    constexpr static CON_TYPE APPLICATION_JSON = "application/json"sv;
    constexpr static CON_TYPE APPLICATION_XML = "application/xml"sv;
    constexpr static CON_TYPE APPLICATION_OCTET_STREAM = "application/octet-stream"sv;
    constexpr static CON_TYPE IMAGE_PNG = "image/png"sv;
    constexpr static CON_TYPE IMAGE_JPEG = "image/jpeg"sv;
    constexpr static CON_TYPE IMAGE_GIF = "image/gif"sv;
    constexpr static CON_TYPE IMAGE_BMP = "image/bmp"sv;
    constexpr static CON_TYPE IMAGE_ICO = "image/vnd.microsoft.icon"sv;
    constexpr static CON_TYPE IMAGE_TIFF = "image/tiff"sv;
    constexpr static CON_TYPE IMAGE_SVG = "image/svg+xml"sv;
    constexpr static CON_TYPE AUDIO_MP3 = "audio/mpeg"sv;
};

bool IsSubPath(fs::path path, fs::path base);
ContentType::CON_TYPE GetContentType(const fs::path& file_path);

class APIHandler {
public:
    explicit APIHandler(app::App& app) : app_{app} {}
public:
    StrResp Response(StrReqt &&req);
private:
    // TODO: мб можно сделать коллекцией endpoints
    StrResp JoinGameUseCase(StrReqt &&req);
    StrResp GetGameStateUseCase(StrReqt &&req) const;
    StrResp MovePlayerUseCase(StrReqt &&req);
    StrResp GameTickUseCase(StrReqt &&req);
    StrResp GetPlayersListUseCase(StrReqt &&req) const;
    StrResp GetMapsListUseCase() const;
    StrResp GetMapUseCase(StrReqt &&req) const;
    StrResp GetRecordsUseCase(StrReqt &&req) const;
private:
    static StrResp PrepareHeader(StrResp &&resp);
    static StrResp GoodResponse(std::string_view body);
    static StrResp BadResponse(const http::status& status, const ErrMsg& msg);
    static std::optional<std::string_view> TryExtractToken(const StrReqt &req);
public:
    // TODO: мб переделать на нешаблонную функцию с вектором
    template<typename First, typename... Args>
    static StrResp InvalidMethodResponse(const First method, const Args... other) {
        StrResp resp;
        resp.result(http::status::method_not_allowed);
        std::ostringstream allowed;
        allowed << method;
        ((allowed << ", " << other), ...);
        resp.set(http::field::allow, allowed.str());
        resp.body() = json::serialize(json::value_from(ErrMsg{"invalidMethod", "Another method is expected"}));
        return PrepareHeader(std::move(resp));
    }
private:
    app::App& app_;
};


class RequestHandler {
public:
    explicit RequestHandler(Strand& api_strand, app::App& app, fs::path& static_content_path)
        : api_strand_{api_strand}
        , api_{app}
        , content_root_{static_content_path} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>> &&req, Send &&send) {
        if (req.target().starts_with("/api/"sv)) {
            HandleAPIRequest(std::move(req), std::forward<Send>(send));
        } else {
            HandleContentRequest(std::move(req), std::forward<Send>(send));
        }
    }

private:
    template <typename SendT>
    void HandleAPIRequest(StrReqt &&req, SendT &&send) {
        net::dispatch(api_strand_, [this, req = std::move(req), send = std::forward<SendT>(send)]() mutable {
            send(api_.Response(std::move(req)));
        });
    }

    template <typename SendT>
    void HandleContentRequest(StrReqt &&req, SendT &&send) {
        auto rel_path = fs::path{req.target().substr(1)};
        auto abs_path = fs::weakly_canonical(content_root_ / rel_path);

        if (is_directory(abs_path)) {
            abs_path /= "index.html";
        }

        if (IsSubPath(abs_path, content_root_) && fs::exists(abs_path)) {
            http::file_body::value_type file;
            if (sys::error_code ec; file.open(abs_path.c_str(), beast::file_mode::read, ec), !ec) {
                http::response<http::file_body> resp;
                resp.result(http::status::ok);
                resp.body() = std::move(file);
                resp.set(http::field::content_type, GetContentType(abs_path));
                resp.prepare_payload();
                return send(std::move(resp));
            }
        }

        StrResp resp;
        resp.result(http::status::ok);
        resp.body() = json::serialize(json::value_from(ErrMsg{"fileNotFound", "File not found"}));
        resp.set(http::field::content_type, ContentType::TEXT_PLAIN);
        resp.keep_alive(true);
        resp.prepare_payload();
        return send(std::move(resp));
    }

private:
    // TODO: проверить везде соответствие последовательности полей и списков инициализации
    const Strand& api_strand_;
    APIHandler api_;
    const fs::path& content_root_;
};

}  // namespace http_handler

#endif  // REQUEST_HANDLER_H