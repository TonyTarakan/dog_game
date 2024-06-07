#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <iostream>

#include <boost/serialization/vector.hpp>

#include "app.h"
#include "geom.h"
#include "model_dog.h"
#include "model.h"

namespace model {

template <typename Archive>
void serialize(Archive& ar, Point2D& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

template <typename Archive>
void serialize(Archive& ar, Vec2D& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.dx;
    ar& vec.dy;
}

template <typename Archive>
void serialize(Archive& ar, CargoItem& obj, [[maybe_unused]] const unsigned version) {
    ar&(obj.id);
    ar&(obj.type);
}

template <typename Archive>
void serialize(Archive& ar, Map::Id& obj, [[maybe_unused]] const unsigned version) {
    ar&(*obj);
}

template <typename Archive>
void serialize(Archive& ar, LootItem& obj, [[maybe_unused]] const unsigned version) {
    ar&(obj.id);
    ar&(obj.type);
    ar&(obj.pos);
}

template <typename Archive>
void serialize(Archive& ar, Dog::Id& obj, [[maybe_unused]] const unsigned version) {
    ar& (*obj);
}

}  // namespace model

namespace serialization {

// DogRepr (DogRepresentation) - сериализованное представление класса Dog
class DogRepr {
public:
    DogRepr() = default;

    explicit DogRepr(const model::Dog& dog)
        : id_(dog.GetIdValue())
        , name_(dog.GetName())
        , pos_(dog.GetPosition())
        , bag_capacity_(dog.GetBagCapacity())
        , speed_(dog.GetSpeed())
        , direction_(dog.GetDirection())
        , score_(dog.GetScore())
        , bag_content_(dog.GetBagContent()) {
    }

    [[nodiscard]] model::Dog Restore() const {
        model::Dog dog{id_, name_, pos_, bag_capacity_};
        dog.SetSpeed(speed_);
        dog.SetDirection(direction_);
        dog.AddScore(score_);
        for (const auto& item : bag_content_) {
            if (!dog.PutToBag(item)) {
                throw std::runtime_error("Failed to put bag content");
            }
        }
        return dog;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& id_;
        ar& name_;
        ar& pos_;
        ar& bag_capacity_;
        ar& speed_;
        ar& direction_;
        ar& score_;
        ar& bag_content_;
    }

private:
    model::Dog::Id::ValueType id_ = 0u;
    std::string name_;
    model::Point2D pos_;
    size_t bag_capacity_ = 0;
    model::Vec2D speed_;
    model::Direction direction_ = model::Direction::NORTH;
    unsigned score_ = 0;
    std::vector<model::CargoItem> bag_content_;
};

class GameSessionRepr {
public:
    GameSessionRepr() = default;

    explicit GameSessionRepr(const model::GameSession& game_session)
            : map_id_val_(*game_session.GetMapId())
            , id_val_(game_session.GetIdValue()) {
        for (const auto& [id, loot] : game_session.GetLoots()) {
            loots_.emplace_back(loot);
        }
        for (const auto& dog : game_session.GetDogs()) {
            dogs_repr_.emplace_back(dog);
        };
    }

    [[nodiscard]] std::shared_ptr<model::GameSession> Restore(std::shared_ptr<model::Game> game) const {
        model::Map::Id map_id{map_id_val_};
        auto game_session = std::make_shared<model::GameSession>(id_val_, game, map_id);

        std::map<uint64_t, model::LootItem> loots_to_set;
        for (const auto& loot : loots_) {
            loots_to_set[loot.id] = loot;
        }
        game_session->SetLoots(loots_to_set);

        for (const auto& dog : dogs_repr_) {
            game_session->AddDog(dog.Restore());
        }

        return game_session;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& id_val_;
        ar& map_id_val_;
        ar& loots_;
        ar& dogs_repr_;
    }

private:
    model::GameSession::Id::ValueType id_val_;
    model::Map::Id::ValueType map_id_val_;
    std::vector<model::LootItem> loots_;
    std::vector<DogRepr> dogs_repr_;
};

class GameRepr {
public:
    GameRepr() = default;
    explicit GameRepr(std::shared_ptr<model::Game> game) {
       for (const auto& [id, sess] : game->GetSessions()) {
           sessions_.emplace_back(*sess);
       }
    }

    void Restore(std::shared_ptr<model::Game> game) const {
        for (const auto& sess : sessions_) {
            game->AddSession(sess.Restore(game));
        }
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& sessions_;
    }

private:
    std::vector<GameSessionRepr> sessions_;
};

class PlayerRepr {
public:
    PlayerRepr() = default;
    explicit PlayerRepr(const app::Player& player)
        : id_val_(player.GetIdValue())
        , sess_id_val_(*player.GetSessionId())
        , name_(player.GetDogName())
        , token_(player.GetTokenValue()) {
    }

    [[nodiscard]] app::Player Restore() const {
        app::Player::Id id{id_val_};
        model::GameSession::Id sess_id{sess_id_val_};
        app::Player player{id, sess_id, name_, token_};
        return player;
    }

    template<typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& id_val_;
        ar& sess_id_val_;
        ar& name_;
        ar& token_;
    }
private:
    app::Player::Id::ValueType id_val_;
    model::GameSession::Id::ValueType sess_id_val_;
    std::string name_;
    app::Token token_;
};

class AllPlayersRepr {
public:
    AllPlayersRepr() = default;
    explicit AllPlayersRepr(const app::Players& players) {
        for (const auto& [id, player] : players.GetPlayers()) {
            players_.emplace_back(*player);
        }
    }

    [[nodiscard]] app::Players Restore() const {
        app::Players players;
        for (const auto& player : players_) {
            app::Player p = player.Restore();
            players.Add(p.GetDogName(), p.GetSessionId(), p.GetIdValue(), p.GetTokenValue());
        }
        return players;
    }
    template<typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& players_;
    }
private:
    std::vector<PlayerRepr> players_;
};

class AppRepr {
public:
    AppRepr() = default;
    explicit AppRepr(app::App& app)
        : game_(app.GetGame())
        , players_(app.GetPlayers()) {
    }

    void Restore(app::App& app) const {
        auto game_ptr = app.GetGame();
        game_.Restore(game_ptr);
        app.RestorePlayers(players_.Restore());
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& game_;
        ar& players_;
    }
private:
    GameRepr game_;
    AllPlayersRepr players_;
};

}  // namespace serialization

#endif  // SERIALIZATION_H