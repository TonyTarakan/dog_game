#ifndef GAME_SERVER_MODEL_H
#define GAME_SERVER_MODEL_H

#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/signals2.hpp>

#include "geom.h"
#include "loot.h"
#include "model_dog.h"
#include "model_geometry.h"
#include "tagged.h"

namespace model {

class Game;

// TODO: use CargoItem as LootItem member, rename
struct LootItem {
    uint64_t id;
    unsigned type;
    Point2D pos;
    bool operator<(const LootItem& other) const { return id < other.id; }
};

struct GameState {
    std::map<std::string, model::Dog> actors;
    std::map<std::string, model::LootItem> loots;
};

using LootGenPtr = std::shared_ptr<loot::LootGenerator>;

namespace net = boost::asio;
namespace sig = boost::signals2;

class GameSession {
public:
    using Id = util::Tagged<uint64_t, GameSession>;
    explicit GameSession(Id::ValueType id, std::shared_ptr<Game> game, Map::Id map_id);
public:
    void AddDog(Id::ValueType id, const std::string &name);
    void AddDog(const Dog& dog);
    void RemoveDog(Dog::Id::ValueType id);
    void SetDogDirection(Dog::Id::ValueType id, Direction direction);
    [[nodiscard]] Id::ValueType GetIdValue() const;
    [[nodiscard]] Map::Id GetMapId() const;
    [[nodiscard]] std::vector<Dog> GetDogs() const;
    [[nodiscard]] std::map<uint64_t, LootItem> GetLoots() const;
    void SetLoots(const std::map<uint64_t, LootItem>& loots);
    void Tick(double tick_duration_ms);
private:
    [[nodiscard]] Point2D GeneratePosition() const;
    void MoveDog(Dog& dog, double tick_ms);
    void MoveAllDogs(double tick_ms);
    void AddLoots(unsigned count);
    [[nodiscard]] unsigned GetRandomLootTypeId() const;
private:
    Id id_;
    std::vector<Dog> dogs_ = {};
    Map::Id map_id_;
    std::shared_ptr<Game> game_;
    uint64_t loot_max_id_ = 1;
    std::map<uint64_t, LootItem> loots_ = {};
    loot::MapLootTypes loot_data_;
};

class Game : public std::enable_shared_from_this<Game> {
    using TickSignal = sig::signal<void(std::chrono::milliseconds delta)>;
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using Maps = std::vector<Map>;
    using LootData = std::map<Map::Id, loot::MapLootTypes>;
    using Sessions = std::map<GameSession::Id::ValueType, std::shared_ptr<GameSession>>;
public:
    void AddMap(const Map& map);
    [[nodiscard]] std::shared_ptr<Map> FindMap(const Map::Id::ValueType& id) const;
    [[nodiscard]] std::shared_ptr<GameSession> GetSession(const Map& map);
    [[nodiscard]] Sessions GetSessions() const;
    [[nodiscard]] Maps GetMaps() const;
    [[nodiscard]] double GetDefaultSpeed() const;
    void SetDefaultSpeed(double speed);
    [[nodiscard]] size_t GetDefaultBagSize() const;
    void SetDefaultBagSize(size_t bag_size);
    void ExternalTick(std::chrono::milliseconds tick_ms);
    void TickAllSessions(const std::chrono::milliseconds& tick_ms);
    void SetRandomSpawn(bool random_spawn);
    [[nodiscard]] bool HasRandomSpawn() const;
    void SetLootGenerator(LootGenPtr loot_gen);
    [[nodiscard]] LootGenPtr GetLootGenerator() const;
    void SetLootData(const LootData& loot_data);
    [[nodiscard]] loot::MapLootTypes GetLootData(const Map::Id& map_id) const;
    void AddSession(std::shared_ptr<GameSession> session);
    [[nodiscard]] sig::connection DoOnTick(const TickSignal::slot_type& handler);
    [[nodiscard]] std::chrono::milliseconds GetRetirementTime() const;
    void SetRetirementTime(double retirement_seconds);
private:
    std::shared_ptr<GameSession> AddSession(const Map& map);
    std::shared_ptr<GameSession> FindSession(const Map& map);
private:
    std::vector<Map> maps_;
    Sessions sessions_;
    MapIdToIndex map_id_to_index_;
    double default_speed_val_ = 0.0;
    size_t default_bag_size_ = 3;
    bool start_from_random_place_ = false;
    LootGenPtr loot_generator_;
    LootData loot_data_;
    TickSignal tick_signal_;
    std::chrono::milliseconds retirement_time_ = std::chrono::milliseconds(60'000);
};

}  // namespace model

#endif  // GAME_SERVER_MODEL_H