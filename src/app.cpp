#include <format>
#include "app.h"

namespace app {

/*
 * Player methods
 */
Token Player::GetTokenValue() const {
    return token_;
}

Player::Id::ValueType Player::GetIdValue() const {
    return *id_;
}

std::string Player::GetDogName() const {
    return dog_name_;
}

model::GameSession::Id Player::GetSessionId() const {
    return session_id_;
}

Token Player::GenerateToken() {
    std::random_device rdev;
    std::mt19937_64 gen(rdev());
    std::uniform_int_distribution<std::mt19937_64::result_type> dist;
    return {std::format("{:0>16x}{:0>16x}", dist(gen), dist(gen))};
}

/*
 * Players methods
 */
std::shared_ptr<Player> Players::Add(const std::string& dog_name,
                                     model::GameSession::Id sess_id,
                                     std::optional<Player::Id::ValueType> id,
                                     std::optional<Token> token) {
    auto new_player_id_val = id ? *id : std::max_element(players_map_.begin(), players_map_.end())->first + 1;
    Player::Id new_player_id{new_player_id_val};
    auto new_player = std::make_shared<Player>(new_player_id, sess_id, dog_name, token);
    players_map_.emplace(*new_player_id, new_player);
    token_to_player_.emplace(new_player->GetTokenValue(), new_player);
    return new_player;
}

std::map<Player::Id::ValueType, std::shared_ptr<Player>> Players::GetPlayers() const {
    return players_map_;
}

std::optional<std::shared_ptr<Player>> Players::GetPlayer(std::string_view token) const {
    if (token_to_player_.find(Token{token}) == token_to_player_.end()) {
        return std::nullopt;
    }
    return token_to_player_.at(Token{token});
}

void Players::DeletePlayer(Player::Id::ValueType id) {
    token_to_player_.erase(players_map_.at(id)->GetTokenValue());
    players_map_.erase(id);
}

/*
 * App methods
 */
std::shared_ptr<model::Game> App::GetGame() const {
    return game_;
}

std::pair<uint64_t, std::string> App::JoinGame(const std::string& username, model::Map& map) {
    auto session = game_->GetSession(map);
    auto sess_id = model::GameSession::Id{session->GetIdValue()};
    auto player = players_.Add(username, sess_id);
    session->AddDog(player->GetIdValue(), username);
    return {player->GetIdValue(), player->GetTokenValue()};
}

std::optional<std::shared_ptr<Player>> App::GetPlayer(std::string_view token) const {
    return players_.GetPlayer(token);
}

Players App::GetPlayers() const {
    return players_;
}

void App::RestorePlayers(const Players& players) {
    players_ = players;
}

std::map<std::string, std::string> App::GetPlayersInfo() const {
    std::map<std::string, std::string> players_info;
    for (const auto& [id, player] : players_.GetPlayers()) {
        players_info[std::to_string(id)] = R"({"name":")" + player->GetDogName() + R"("})";
    }
    return players_info;
}

model::GameState App::GetGameState(std::string_view token) const {
    std::map<std::string, model::Dog> dogs;
    auto player_ptr = *players_.GetPlayer(token);
    auto session = game_->GetSessions().at(*player_ptr->GetSessionId());
    for (const auto& dog : session->GetDogs()) {
        dogs.emplace(std::to_string(dog.GetIdValue()), dog);
    }
    std::map<std::string, model::LootItem> loots;
    for (const auto& [item_id, item] : session->GetLoots()) {
        loots.emplace(std::to_string(item_id), item);
    }
    return {dogs, loots};
}

void App::RetireDogs() {
    auto retirement_time = GetGame()->GetRetirementTime();
    for (auto & [sess_id, sess_ptr] : GetGame()->GetSessions()) {
        std::vector<model::Dog::Id::ValueType> retired_dog_ids;
        for (const auto &dog : sess_ptr->GetDogs()) {
            if (dog.GetNonactiveTime() >= retirement_time) {
                db_.Save({
                                 dog.GetName(),
                                 dog.GetScore(),
                                 dog.GetPlayTime()
                         });
                retired_dog_ids.push_back(dog.GetIdValue());
            }
        }
        for (auto id : retired_dog_ids) {
            sess_ptr->RemoveDog(id);
            players_.DeletePlayer(id);
        }
    }
}

domain::Retirees App::GetRetiredDogs(std::optional<int> start, std::optional<int> max_items) const {
    return db_.GetRetiredDogs(start, max_items);
}

} // namespace app