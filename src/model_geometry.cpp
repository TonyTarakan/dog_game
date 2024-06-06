#include <cmath>
#include <stdexcept>

#include "model_geometry.h"

namespace model {

/*
 * Point methods
 */
bool Point::operator==(const Point& other) const {
    return x == other.x && y == other.y;
}


/*
 * GeoRectangle methods
 */
bool GeoRectangle::Contains(const Point2D& point) const {
    return (point.x >= min_corner.x
            && point.x <= max_corner.x
            && point.y >= min_corner.y
            && point.y <= max_corner.y);
}

Point2D GeoRectangle::LeavingPoint(const GeoSegment& vec) const {
    if (vec.end.x > vec.start.x) {
        return {max_corner.x, vec.start.y};
    }
    if (vec.end.y > vec.start.y) {
        return {vec.start.x, max_corner.y};
    }
    if (vec.end.x < vec.start.x) {
        return {min_corner.x, vec.start.y};
    }
    if (vec.end.y < vec.start.y) {
        return {vec.start.x, min_corner.y};
    }

    throw std::logic_error("Unexpected segment: "
                           + std::to_string(vec.start.x) + ", " + std::to_string(vec.start.y) + " -> "
                           + std::to_string(vec.end.x) + ", " + std::to_string(vec.end.y));
}

/*
 * Road methods
 */
bool Road::IsHorizontal() const {
    return start_.y == end_.y;
}

Point Road::GetStart() const {
    return start_;
}

Point Road::GetEnd() const {
    return end_;
}

GeoRectangle Road::GetBounds() const {
    return bounds_;
}

bool Road::operator==(const Road& other) const {
    return start_ == other.start_ && end_ == other.end_;
}

/*
 * Building methods
 */
Rectangle Building::GetBounds() const {
    return bounds_;
}

/*
 * Office methods
 */
Office::Id Office::GetId() const {
    return id_;
}

Point Office::GetPosition() const {
    return position_;
}

Offset Office::GetOffset() const {
    return offset_;
}

/*
 * Map methods
 */
Map::Id Map::GetId() const {
    return id_;
}

std::string Map::GetName() const {
    return name_;
}

Map::Buildings Map::GetBuildings() const {
    return buildings_;
}

Map::Roads Map::GetRoads() const {
    return roads_;
}

Map::Offices Map::GetOffices() const {
    return offices_;
}

void Map::AddRoad(const Road& road) {
    roads_.emplace_back(road);
}

void Map::AddBuilding(const Building& building) {
    buildings_.emplace_back(building);
}

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

void Map::SetSpeed(double speed) {
    speed_val_ = speed;
}

double Map::GetSpeed() const {
    return speed_val_;
}

/*
 * MapShortView methods
 */
Map::Id MapShortView::GetId() const {
    return map_.GetId();
}

std::string MapShortView::GetName() const {
    return map_.GetName();
}

}  // namespace model
