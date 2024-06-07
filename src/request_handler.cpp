#include <filesystem>
#include <optional>
#include <string_view>

#include <boost/json.hpp>
#include <boost/system.hpp>
#include <boost/beast.hpp>
#include <boost/regex.hpp>

#include "request_handler.h"
#include "handler_serializer.h"

namespace fs = std::filesystem;
namespace beast = boost::beast;
namespace json = boost::json;
namespace http = beast::http;
namespace sys = boost::system;

namespace http_handler {

bool IsSubPath(fs::path path, fs::path base) {
    // Приводим оба пути к каноничному виду (без . и ..)
    path = fs::weakly_canonical(path);
    base = fs::weakly_canonical(base);

    // Проверяем, что все компоненты base содержатся внутри path
    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

ContentType::CON_TYPE GetContentType(const fs::path& file_path) {
    // TODO: replace to cache-friendly flat_map
    static const std::map<fs::path::string_type, ContentType::CON_TYPE> ext_to_ct_map = {
        {".html", ContentType::TEXT_HTML},
        {".css", ContentType::TEXT_CSS},
        {".txt", ContentType::TEXT_PLAIN},
        {".js", ContentType::TEXT_JS},
        {".json", ContentType::APPLICATION_JSON},
        {".xml", ContentType::APPLICATION_XML},
        {".png", ContentType::IMAGE_PNG},
        {".jpeg", ContentType::IMAGE_JPEG},
        {".jpg", ContentType::IMAGE_JPEG},
        {".gif", ContentType::IMAGE_GIF},
        {".bmp", ContentType::IMAGE_BMP},
        {".ico", ContentType::IMAGE_ICO},
        {".tiff", ContentType::IMAGE_TIFF},
        {".svg", ContentType::IMAGE_SVG},
        {".mp3", ContentType::AUDIO_MP3}
    };

    const auto search_res = ext_to_ct_map.find(file_path.extension());
    if (search_res != ext_to_ct_map.end()) {
        return search_res->second;
    }

    return ContentType::APPLICATION_OCTET_STREAM;
}

/*
 * API methods
 */
StrResp APIHandler::PrepareHeader(StrResp &&resp) {
    resp.set(http::field::cache_control, "no-cache");
    resp.set(http::field::content_type, ContentType::APPLICATION_JSON);
    resp.keep_alive(true);
    resp.prepare_payload();
    return resp;
}

StrResp APIHandler::GoodResponse(std::string_view body) {
    StrResp resp;
    resp.result(http::status::ok);
    resp.body() = body;
    return PrepareHeader(std::move(resp));
}

StrResp APIHandler::BadResponse(const http::status& status, const ErrMsg& msg) {
    StrResp resp;
    resp.result(status);
    resp.body() = json::serialize(json::value_from(msg));
    return PrepareHeader(std::move(resp));
}

std::optional<std::string_view> APIHandler::TryExtractToken(const StrReqt &req) {
    auto auth_field = req.find(http::field::authorization);
    if (auth_field == req.end()) {
        return std::nullopt;
    }

    auto token_bearer = auth_field->value();
    const std::string_view bearer_prefix = "Bearer ";
    if (!token_bearer.starts_with(bearer_prefix)) {
        return std::nullopt;
    }

    auto token = token_bearer.substr(bearer_prefix.size());
    if (token.length() != app::TOKEN_LENGTH) {
        return std::nullopt;
    }

    return token;
}

StrResp APIHandler::GetMapsListUseCase() const {
    auto maps = app_.GetGame()->GetMaps();
    std::vector<model::MapShortView> map_views{maps.begin(), maps.end()};
    return GoodResponse(json::serialize(json::value_from(map_views)));
}

StrResp APIHandler::GetMapUseCase(StrReqt &&req) const {
    auto url = req.target();
    const std::string maps_url = "/api/v1/maps";

    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        return InvalidMethodResponse(http::verb::get, http::verb::head);
    }

    if (url == maps_url) {
        return GetMapsListUseCase();
    }

    std::string map_id_str{url.substr(maps_url.length() + 1)};

    auto map = app_.GetGame()->FindMap(map_id_str);
    if (map) {
        auto loot = app_.GetGame()->GetLootData(map->GetId());
        return GoodResponse(json::serialize(json::value_from(std::make_pair(*map, loot))));
    }

    return BadResponse(http::status::not_found, {"mapNotFound", "Map not found"});
}

// TODO: Check for library function
static std::optional<int> GetUrlParam(const std::string& params, const std::string& name) {
    boost::regex expr{"(\\?|&|^|,)"s + name + "=(\\w+)(\\&|$)"s };
    boost::smatch what;
    if (boost::regex_search(params, what, expr)) {
        return std::stoi(what[2]);
    }
    return std::nullopt;
}

StrResp APIHandler::GetRecordsUseCase(http_handler::StrReqt &&req) const {

    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        return InvalidMethodResponse(http::verb::get, http::verb::head);
    }

    std::string url{req.target()};

    auto req_start_param = GetUrlParam(url, "start");
    auto req_count_param = GetUrlParam(url, "maxItems");

    if (req_count_param.has_value()) {
        if (req_count_param > 100) {
            return BadResponse(http::status::bad_request, {"invalidArgument", "Too many items"});
        }
    }

    auto records = app_.GetRetiredDogs(req_start_param, req_count_param);
    return GoodResponse(json::serialize(json::value_from(records)));
}

StrResp APIHandler::JoinGameUseCase(StrReqt &&req) {

    if (req.method() != http::verb::post) {
        return InvalidMethodResponse(http::verb::post);
    }

    try {
        const auto req_json = json::parse(req.body());
        std::string username{req_json.at("userName").as_string()};
        const std::string map_id_str{req_json.at("mapId").as_string()};

        if (username.empty()) {
            return BadResponse(http::status::bad_request, {"invalidArgument", "Invalid name"});
        }

        auto map = app_.GetGame()->FindMap(map_id_str);
        if (!map) {
            return BadResponse(http::status::not_found, {"mapNotFound", "Map not found"});
        }

        const auto [player_id, auth_token] = app_.JoinGame(username, *map); // TODO: отвязать от модели
        return GoodResponse(json::serialize(json::value_from(JoinMsg{player_id, auth_token})));

    } catch (const std::exception& e) {
        return BadResponse(http::status::bad_request, {"invalidArgument", "Join game request parse error"});
    }
}

StrResp APIHandler::GetGameStateUseCase(StrReqt &&req) const {

    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        return InvalidMethodResponse(http::verb::get, http::verb::head);
    }

    if (auto token = TryExtractToken(req)) {
        if (app_.GetPlayer(*token)) {
            return GoodResponse(json::serialize(json::value_from(app_.GetGameState(*token))));
        }
        return BadResponse(http::status::unauthorized, {"unknownToken", "Player token has not been found"});
    }

    return BadResponse(http::status::unauthorized, {"invalidToken", "Authorization header has wrong format"});
}

StrResp APIHandler::MovePlayerUseCase(StrReqt &&req) {

    if (req.method() != http::verb::post) {
        return InvalidMethodResponse(http::verb::post);
    }

    if (auto token = TryExtractToken(req)) {
        if (auto player_opt = app_.GetPlayer(*token)) {
            try {
                auto req_json = json::parse(req.body());
                auto dir_str = req_json.at("move").as_string().c_str();
                auto player_ptr = player_opt.value();

                const std::map<std::string, model::Direction> dir_map{
                    {"U", model::Direction::NORTH},
                    {"D", model::Direction::SOUTH},
                    {"L", model::Direction::WEST},
                    {"R", model::Direction::EAST},
                    {"", model::Direction::NONE}
                };

                if (dir_map.find(dir_str) == dir_map.end()) {
                    throw std::runtime_error("Invalid direction");
                }

                // TODO: check and maybe refactor
                auto sess_id = player_ptr->GetSessionId();
                auto session = app_.GetGame()->GetSessions().at(*sess_id);
                session->SetDogDirection(player_ptr->GetIdValue(), dir_map.at(dir_str));
                return GoodResponse("{}");

            } catch (const std::exception& e) {
                return BadResponse(http::status::bad_request, {"invalidArgument", "Action parse error"});
            }
        }
        return BadResponse(http::status::unauthorized, {"unknownToken", "Player token has not been found"});
    }
    return BadResponse(http::status::unauthorized, {"invalidToken", "Authorization header has wrong format"});
}

StrResp APIHandler::GameTickUseCase(StrReqt &&req) {

    if (req.method() != http::verb::post) {
        return InvalidMethodResponse(http::verb::post);
    }

    try {
        const auto req_json = json::parse(req.body());
        const auto tick_ms = req_json.at("timeDelta").as_int64();
        app_.GetGame()->ExternalTick(std::chrono::milliseconds(tick_ms));
        return GoodResponse("{}");

    } catch (const std::exception& e) {
        return BadResponse(http::status::bad_request, {"invalidArgument", "JSON parse error"});
    }
}

StrResp APIHandler::GetPlayersListUseCase(StrReqt &&req) const {

    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        return InvalidMethodResponse(http::verb::get, http::verb::head);
    }

    if (auto token = TryExtractToken(req)) {
        if (app_.GetPlayer(*token)) {
            return GoodResponse(json::serialize(json::value_from(app_.GetPlayersInfo())));
        }
        return BadResponse(http::status::unauthorized, {"unknownToken", "Player token has not been found"});
    }

    return BadResponse(http::status::unauthorized, {"invalidToken", "Authorization header is missing"});
}

StrResp APIHandler::Response(StrReqt &&req) {
    auto url = req.target();

    // TODO: некрасиво, конечно...
    if (url == "/api/v1/game/state") {
        return GetGameStateUseCase(std::move(req));
    }

    if (url == "/api/v1/game/player/action") {
        return MovePlayerUseCase(std::move(req));
    }

    if (url.starts_with("/api/v1/maps")) {
        return GetMapUseCase(std::move(req));
    }

    if (url == "/api/v1/game/join") {
        return JoinGameUseCase(std::move(req));
    }

    if (url == "/api/v1/game/players") {
        return GetPlayersListUseCase(std::move(req));
    }

    if (url == "/api/v1/game/tick") {
        return GameTickUseCase(std::move(req));
    }

    if (url.starts_with("/api/v1/game/records")) {
        return GetRecordsUseCase(std::move(req));
    }

    return BadResponse(http::status::bad_request, {"badRequest", "Bad request"});
}

}  // namespace http_handler
