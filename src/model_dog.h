#ifndef GAME_SERVER_MODEL_DOG_H
#define GAME_SERVER_MODEL_DOG_H

#include <chrono>
#include <deque>
#include "tagged.h"
#include "geom.h"
#include "model_geometry.h"

namespace model {

struct CargoItem {
    using Id = util::Tagged<uint32_t, CargoItem>;

    uint64_t id{0u};
    unsigned type{0u};

    [[nodiscard]] auto operator<=>(const CargoItem&) const = default;
};

class Dog {
public:
    using Id = util::Tagged<uint64_t, Dog>;
public:
    explicit Dog(Id::ValueType id, std::string name, Point2D pos, size_t bag_cap) noexcept
            : id_{id}
            , name_(std::move(name))
            , position_(pos)
            , bag_cap_(bag_cap) {
        bag_.reserve(bag_cap);
    }
public:
    [[nodiscard]] Id::ValueType GetIdValue() const;
    [[nodiscard]] std::string GetName() const;
    [[nodiscard]] Point2D GetPosition() const;
    void SetPosition(const Point2D& position);
    [[nodiscard]] Vec2D GetSpeed() const;
    void SetSpeed(const Vec2D& speed);
    [[nodiscard]] Direction GetDirection() const;
    void SetDirection(const Direction& direction);
    [[nodiscard]] size_t GetBagCapacity() const;
    [[nodiscard]] bool IsBagFull() const;
    [[nodiscard]] bool PutToBag(const CargoItem& item);
    size_t EmptyBag();
    [[nodiscard]] std::vector<CargoItem> GetBagContent() const;
    [[nodiscard]] unsigned GetScore() const;
    void AddScore(unsigned points);
    [[nodiscard]] std::chrono::milliseconds GetNonactiveTime() const;
    void AddNonactiveTime(std::chrono::milliseconds time);
    void ResetNonactiveTime();
    void AddPlayTime(std::chrono::milliseconds time);
    [[nodiscard]] std::chrono::milliseconds GetPlayTime() const;
private:
    Id id_;
    std::string name_;
    Point2D position_;
    size_t bag_cap_;
    model::Vec2D speed_ = {0.0, 0.0};
    Direction direction_ = Direction::NONE;
    std::vector<CargoItem> bag_;
    unsigned score_ = 0;
    std::chrono::milliseconds play_time = std::chrono::milliseconds(0);
    std::chrono::milliseconds nonactive_time = std::chrono::milliseconds(0);
};

} // namespace model

#endif //GAME_SERVER_MODEL_DOG_H
