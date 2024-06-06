#ifndef GAME_SERVER_APP_H
#define GAME_SERVER_APP_H

#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <utility>
#include <boost/asio/io_context.hpp>
#include "model.h"

#include "domain.h"
#include "db.h"

namespace app {

namespace net = boost::asio;
using Token = std::string;
constexpr unsigned TOKEN_LENGTH = 32;

class Player {
public:
    using Id = util::Tagged<uint64_t, Player>;
    explicit Player(Id id,
           model::GameSession::Id sess_id,
           std::string dog_name,
           std::optional<Token> token = std::nullopt)
        : id_(id)
        , session_id_(sess_id)
        , dog_name_(std::move(dog_name)) {
        token_ = token ? Token(*token) : GenerateToken();
    }
public:
    [[nodiscard]] Token GetTokenValue() const;
    [[nodiscard]] Id::ValueType GetIdValue() const;
    [[nodiscard]] std::string GetDogName() const;
    [[nodiscard]] model::GameSession::Id GetSessionId() const;
private:
    static Token GenerateToken();
private:
    Id id_;
    model::GameSession::Id session_id_;
    std::string dog_name_;
    Token token_;
};

class Players {
public:
    std::shared_ptr<Player> Add(const std::string& dog_name,
                                model::GameSession::Id sess_id,
                                std::optional<Player::Id::ValueType> id = std::nullopt,
                                std::optional<Token> token = std::nullopt);
    [[nodiscard]] std::map<Player::Id::ValueType, std::shared_ptr<Player>> GetPlayers() const;
    [[nodiscard]] std::optional<std::shared_ptr<Player>> GetPlayer(std::string_view token) const;
    void DeletePlayer(Player::Id::ValueType id);
private:
    std::map<Player::Id::ValueType, std::shared_ptr<Player>> players_map_;
    std::unordered_map<Token, std::shared_ptr<Player>> token_to_player_;
};

class App {
public:
    explicit App(std::shared_ptr<model::Game> game, db::RecordDB& db)
        : game_(std::move(game))
        , db_{db} {
    }
public:
    [[nodiscard]] std::shared_ptr<model::Game> GetGame() const;
    [[nodiscard]] std::pair<uint64_t, std::string> JoinGame(const std::string& username, model::Map& map);
    [[nodiscard]] std::optional<std::shared_ptr<Player>> GetPlayer(std::string_view token) const;
    [[nodiscard]] Players GetPlayers() const;
    void RestorePlayers(const Players& players);
    [[nodiscard]] std::map<std::string, std::string> GetPlayersInfo() const;
    [[nodiscard]] model::GameState GetGameState(std::string_view token) const;
    void RetireDogs();
    [[nodiscard]] domain::Retirees GetRetiredDogs(std::optional<int> start, std::optional<int> max_items) const;
private:
    std::shared_ptr<model::Game> game_;
    Players players_;
    db::RecordDB& db_;
};

} // namespace app

#endif //GAME_SERVER_APP_H
