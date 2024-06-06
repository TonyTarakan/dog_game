#include <random>
#include <stdexcept>
#include <utility>

#include "collision_detector.h"
#include "model.h"
#include "model_dog.h"
#include "model_geometry.h"

namespace model {
using namespace std::literals;

constexpr double LOOT_WIDTH = 0.0;
constexpr double DOG_WIDTH = 0.6 / 2.0;
constexpr double OFFICE_WIDTH = 0.5 / 2.0;

constexpr int MSEC_IN_SEC = 1000;

/*
 * GameSession methods
 */
GameSession::GameSession(Id::ValueType id, std::shared_ptr<Game> game, Map::Id map_id)
    : id_(id)
    , game_{std::move(game)}
    , map_id_{std::move(map_id)} {
    loot_data_ = game_->GetLootData(map_id_);
}

void GameSession::AddDog(Dog::Id::ValueType id, const std::string &name) {
    Dog dog{id, name, GeneratePosition(), game_->FindMap(*map_id_)->GetBagSize()};
    dog.SetDirection(Direction::NORTH);
    dogs_.emplace_back(dog);
}

void GameSession::AddDog(const Dog& dog) {
    dogs_.push_back(dog);
}

void GameSession::RemoveDog(Dog::Id::ValueType id) {
    for (auto it = dogs_.begin(); it != dogs_.end(); ++it) {
        if (it->GetIdValue() == id) {
            dogs_.erase(it);
            break;
        }
    }
}

void GameSession::SetDogDirection(Dog::Id::ValueType id, Direction direction) {
    auto dog_it = std::find_if(dogs_.begin(), dogs_.end(), [&id](const Dog& dog) {
        return dog.GetIdValue() == id;
    });

    if (dog_it != dogs_.end()) {

        if (direction == Direction::NONE) {
            dog_it->SetSpeed({0, 0});
            return;
        }

        dog_it->SetDirection(direction);
        const auto s = game_->FindMap(*map_id_)->GetSpeed();
        switch (direction) {
            case Direction::NORTH:
                dog_it->SetSpeed({0, -s});
                return;
            case Direction::SOUTH:
                dog_it->SetSpeed({0, s});
                return;
            case Direction::WEST:
                dog_it->SetSpeed({-s, 0});
                return;
            case Direction::EAST:
                dog_it->SetSpeed({s, 0});
                return;
            default:
                throw std::runtime_error("Unknown direction");
        }
    }

    throw std::runtime_error("Dog not found");
}

GameSession::Id::ValueType GameSession::GetIdValue() const {
    return *id_;
}

Map::Id GameSession::GetMapId() const {
    return map_id_;
}

std::vector<Dog> GameSession::GetDogs() const {
    return dogs_;
}

std::map<uint64_t, LootItem> GameSession::GetLoots() const {
    return loots_;
}

void GameSession::SetLoots(const std::map<uint64_t, LootItem>& loots) {
    loots_ = loots;
    if (!loots_.empty()) {
        loot_max_id_ = loots_.rbegin()->first;
    }
}

void GameSession::Tick(double tick_duration_ms) {
    // remember start positions
    using namespace collision_detector;
    std::vector<Gatherer> gatherers;
    for (const auto& dog : dogs_) {
        Gatherer g;
        g.start_pos = {dog.GetPosition().x, dog.GetPosition().y};
        g.width = DOG_WIDTH;
        gatherers.emplace_back(g);
    }

    MoveAllDogs(tick_duration_ms);

    // remember end positions
    for (size_t i = 0; i < dogs_.size(); ++i) {
        gatherers[i].end_pos = {dogs_[i].GetPosition().x, dogs_[i].GetPosition().y};
    }

    std::vector<Item> item_vec;
    for (const auto& [loot_id, loot_item] : loots_) {
        Item item{
            loot_id,
            {loot_item.pos.x, loot_item.pos.y},
            LOOT_WIDTH
        };
        item_vec.emplace_back(item);
    }
    constexpr uint64_t OFFICE_ITEM_TRAIT = 0;
    for (const auto& office : game_->FindMap(*map_id_)->GetOffices()) {
        Item item{
            OFFICE_ITEM_TRAIT,
            {static_cast<double>(office.GetPosition().x), static_cast<double>(office.GetPosition().y)},
            OFFICE_WIDTH
        };
        item_vec.emplace_back(item);
    }

    Provider provider{item_vec, gatherers};
    for (auto [item_id, gatherer_id, time] : FindSortedGatherEvents(provider)) {
        auto& dog = dogs_.at(gatherer_id);

        if (item_id != OFFICE_ITEM_TRAIT) { // item is loot
            if (loots_.contains(item_id)) {
                if (dog.PutToBag({item_id, loots_.at(item_id).type})) {
                    loots_.erase(item_id);
                }
            }
        } else {
            int dog_item_cnt = static_cast<int>(dog.GetBagContent().size());
            for (int i = dog_item_cnt - 1; i >= 0; --i) {
                auto item_value = loot_data_.at(dog.GetBagContent().at(i).type).value;
                dog.AddScore(item_value.value());
            }
            dog.EmptyBag();
        }
    }

    // add new loots
    std::chrono::milliseconds ms(static_cast<int>(tick_duration_ms));
    auto loot_cnt_to_add = game_->GetLootGenerator()->Generate(ms, loots_.size(), dogs_.size());
    AddLoots(loot_cnt_to_add);
}

Point2D GameSession::GeneratePosition() const {
    const auto& roads = game_->FindMap(*map_id_)->GetRoads();

    if (game_->HasRandomSpawn()) {
        std::random_device rdev;
        std::mt19937 generator(rdev());
        std::uniform_int_distribution<> int_dist(0, static_cast<int>(roads.size() - 1));

        const auto random_road = roads.at(int_dist(generator));
        const auto& start = random_road.GetStart();
        const auto& end = random_road.GetEnd();

        std::uniform_real_distribution<> x_dist(std::min(start.x, end.x), std::max(start.x, end.x));
        double x = x_dist(generator);
        std::uniform_real_distribution<> dist(std::min(start.y, end.y), std::max(start.y, end.y));
        double y = dist(generator);
        return {x, y};
    }

    return {
        static_cast<double>(roads.at(0).GetStart().x),
        static_cast<double>(roads.at(0).GetStart().y)
    };
}

void GameSession::MoveDog(model::Dog &dog, double tick_ms) {

    dog.AddPlayTime(std::chrono::milliseconds{static_cast<unsigned>(tick_ms)});

    auto speed = dog.GetSpeed();
    auto start_position = dog.GetPosition();

    Point2D desired_position = {
        start_position.x + speed.dx * tick_ms / static_cast<double>(MSEC_IN_SEC),
        start_position.y + speed.dy * tick_ms / static_cast<double>(MSEC_IN_SEC)
    };

    if (start_position == desired_position || tick_ms == 0.0) {
        dog.AddNonactiveTime(std::chrono::milliseconds{static_cast<unsigned>(tick_ms)});
        return;
    }

    // collision check
    auto roads = game_->FindMap(*map_id_)->GetRoads();
    auto start_road{std::find_if(roads.begin(), roads.end(), [&start_position](const Road& road) {
        return road.GetBounds().Contains(start_position);
    })};

    if (start_road == roads.end()) {
        throw std::runtime_error("No road found");
    }

    auto start_road_rect = start_road->GetBounds();

    // Не убежали со стартовой дороги
    if (start_road_rect.Contains(desired_position)) {
        dog.SetPosition(desired_position);
        dog.ResetNonactiveTime();
        return;
    }

    // А не забежали ли на другую?
    auto border_point = start_road_rect.LeavingPoint({start_position, desired_position});
    auto another_road = roads.begin();
    while (another_road != roads.end()) { // TODO: кажется, можно оптимизировать - пока отложу
        another_road = std::find_if(another_road, roads.end(), [&border_point, start_road](const Road& road) {
            return (road.GetBounds().Contains(border_point) && road != *start_road);
        });

        if (another_road == roads.end()) {
            break; // Вариантов, куда двигаться, больше нет
        }

        // Желаемая позиция на другой дороге? - отлично
        auto another_road_rect = another_road->GetBounds();
        if (another_road_rect.Contains(desired_position)) {
            dog.SetPosition(desired_position);
            dog.ResetNonactiveTime();
            return;
        }

        // Тупик на другой дороге, продолжаем поиск
        border_point = another_road_rect.LeavingPoint({start_position, desired_position});
        another_road++;
    }

    // Не нашли дорогу - тупик
    dog.SetPosition(border_point);
    dog.ResetNonactiveTime();
    dog.SetSpeed({0.0, 0.0});
}

void GameSession::MoveAllDogs(double tick_ms) {
    for (auto& dog : dogs_) {
        MoveDog(dog, tick_ms);
    }
}

unsigned GameSession::GetRandomLootTypeId() const {
    std::random_device rdev;
    std::mt19937 gen(rdev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, loot_data_.size() - 1);
    return dist(gen);
}

// TODO: refactor loot ids
void GameSession::AddLoots(unsigned count) {
    for (unsigned i = 0; i < count; ++i) {
        loot_max_id_++;
        loots_.insert({loot_max_id_, {loot_max_id_, GetRandomLootTypeId(), GeneratePosition()}});
    }
}

/*
 * Game methods
 */
void Game::AddMap(const Map& map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(map);
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

std::shared_ptr<GameSession> Game::AddSession(const Map& map) {
    auto sess_max_id = sessions_.empty() ? 0 : sessions_.rbegin()->first + 1;
    auto session = std::make_shared<GameSession>(sess_max_id, shared_from_this(), map.GetId());
    sessions_.insert({sess_max_id, session});
    return session;
}

std::shared_ptr<GameSession> Game::FindSession(const Map& map) {
    auto session_it = std::find_if(sessions_.begin(), sessions_.end(), [&map](const auto& session) {
        return session.second->GetMapId() == map.GetId();
    });

    if (session_it != sessions_.end()) {
        return session_it->second;
    }

    return nullptr;
}

std::shared_ptr<Map> Game::FindMap(const Map::Id::ValueType& id) const {
    Map::Id map_id{id};
    if (auto it = map_id_to_index_.find(map_id); it != map_id_to_index_.end()) {
        return std::make_shared<Map>(maps_.at(it->second));
    }
    return nullptr;
}

Game::Maps Game::GetMaps() const {
    return maps_;
}

double Game::GetDefaultSpeed() const {
    return default_speed_val_;
}

void Game::SetDefaultSpeed(double speed) {
    default_speed_val_ = speed;
}

size_t Game::GetDefaultBagSize() const {
    return default_bag_size_;
}

void Game::SetDefaultBagSize(size_t bag_size) {
    default_bag_size_ = bag_size;
}

std::shared_ptr<GameSession> Game::GetSession(const Map& map) {
    auto active_session = FindSession(map);
    if (active_session) {
        return active_session;
    }
    return AddSession(map);
}

Game::Sessions Game::GetSessions() const {
    return sessions_;
}

void Game::ExternalTick(std::chrono::milliseconds tick_ms) {
    TickAllSessions(tick_ms);
}

void Game::TickAllSessions(const std::chrono::milliseconds& tick_ms) {
    auto tick_ms_double = static_cast<double>(tick_ms.count());
    for (auto& [_, session] : sessions_) {
        session->Tick(tick_ms_double);
    }
    tick_signal_(tick_ms);
}

void Game::SetRandomSpawn(bool random_spawn) {
    start_from_random_place_ = random_spawn;
}

bool Game::HasRandomSpawn() const {
    return start_from_random_place_;
}

void Game::SetLootGenerator(LootGenPtr loot_gen) {
    loot_generator_ = std::move(loot_gen);
}

LootGenPtr Game::GetLootGenerator() const {
    return loot_generator_;
}

void Game::SetLootData(const LootData& loot_data) {
    loot_data_ = loot_data;
}

loot::MapLootTypes Game::GetLootData(const Map::Id& map_id) const {
    return loot_data_.at(map_id);
}

void Game::AddSession(std::shared_ptr<GameSession> session) {
    sessions_[session->GetIdValue()] = session;
}

sig::connection Game::DoOnTick(const TickSignal::slot_type& handler) {
    return tick_signal_.connect(handler);
}

std::chrono::milliseconds Game::GetRetirementTime() const {
    return retirement_time_;
}

void Game::SetRetirementTime(double retirement_seconds) {
    retirement_time_ = std::chrono::milliseconds{static_cast<int>(retirement_seconds) * MSEC_IN_SEC};
}

}  // namespace model
