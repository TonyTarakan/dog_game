#ifndef GAME_SERVER_MODEL_GEOMETRY_H
#define GAME_SERVER_MODEL_GEOMETRY_H

#include <optional>
#include <string>

#include "geom.h"
#include "tagged.h"

namespace model {

enum class Direction {
    NORTH,
    SOUTH,
    WEST,
    EAST,
    NONE
};

struct Point {
    int x;
    int y;

    [[nodiscard]] bool operator==(const Point& other) const;
};

struct Size {
    int width, height;
};

struct Offset {
    int dx, dy;
};

struct Rectangle {
    Point position;
    Size size;
};

struct GeoSegment {
    Point2D start;
    Point2D end;
};

struct GeoRectangle {
    Point2D min_corner;
    Point2D max_corner;

    [[nodiscard]] bool Contains(const Point2D& point) const;
    [[nodiscard]] Point2D LeavingPoint(const GeoSegment& vec) const;
};

class Road {
    struct HorizontalTag {
        HorizontalTag() = default;
    };
    struct VerticalTag {
        VerticalTag() = default;
    };
public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};
public:
    Road(Point start, Point end)
            : start_{start}
            , end_{end} {
        bounds_ = {
            {static_cast<double>(std::min(start_.x, end_.x)) - HALF_WIDTH,
             static_cast<double>(std::min(start_.y, end_.y)) - HALF_WIDTH},
            {static_cast<double>(std::max(start_.x, end_.x)) + HALF_WIDTH,
             static_cast<double>(std::max(start_.y, end_.y)) + HALF_WIDTH}
        };
    }
public:
    [[nodiscard]] bool IsHorizontal() const;
    [[nodiscard]] Point GetStart() const;
    [[nodiscard]] Point GetEnd() const;
    [[nodiscard]] GeoRectangle GetBounds() const;
    [[nodiscard]] bool operator==(const Road& other) const;
private:
    static constexpr double HALF_WIDTH = 0.4;
    Point start_;
    Point end_;
    GeoRectangle bounds_{};
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
            : bounds_{bounds} {
    }
public:
    [[nodiscard]] Rectangle GetBounds() const;
private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;
public:
    Office(Id id, Point position, Offset offset) noexcept
            : id_{std::move(id)}
            , position_{position}
            , offset_{offset} {
    }
public:
    [[nodiscard]] Id GetId() const;
    [[nodiscard]] Point GetPosition() const;
    [[nodiscard]] Offset GetOffset() const;
private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;
public:
    Map(Id id, std::string name) noexcept
            : id_(std::move(id))
            , name_(std::move(name)) {
    }
public:
    Id GetId() const;
    std::string GetName() const;
    Buildings GetBuildings() const;
    Roads GetRoads() const;
    Offices GetOffices() const;
    void AddRoad(const Road& road);
    void AddBuilding(const Building& building);
    void AddOffice(Office office);
    void SetSpeed(double speed);
    [[nodiscard]] double GetSpeed() const;
    void SetBagSize(size_t bag_size) { bag_size_ = bag_size; }
    [[nodiscard]] size_t GetBagSize() const { return bag_size_; }
private:
    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    Offices offices_;
    OfficeIdToIndex warehouse_id_to_index_;
    double speed_val_;
    size_t bag_size_ = 3;
};

class MapShortView {
public:
    explicit MapShortView(const Map& map) noexcept
            : map_{map} {
    }
public:
    [[nodiscard]] Map::Id GetId() const;
    [[nodiscard]] std::string GetName() const;
private:
    const Map& map_;
};

} // namespace model

#endif //GAME_SERVER_MODEL_GEOMETRY_H
