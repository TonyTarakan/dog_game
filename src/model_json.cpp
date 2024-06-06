#include <boost/json.hpp>

#include "loot.h"
#include "model.h"
#include "model_json.h"

namespace json = boost::json;

namespace model {

Road tag_invoke(json::value_to_tag<Road> &, const json::value &val) {
    return val.as_object().contains("x1")
        ? Road{
            Point{
                static_cast<int>(val.at("x0").as_int64()),
                static_cast<int>(val.at("y0").as_int64())
            },
            Point{
                static_cast<int>(val.at("x1").as_int64()),
                static_cast<int>(val.at("y0").as_int64())
            }
        }
        : Road{
            Point{
                static_cast<int>(val.at("x0").as_int64()),
                static_cast<int>(val.at("y0").as_int64())
            },
            Point{
                static_cast<int>(val.at("x0").as_int64()),
                static_cast<int>(val.at("y1").as_int64())
            }
        };
}

Building tag_invoke(json::value_to_tag<Building>&, const json::value& val) {
    return Building{
        Rectangle{
            Point{
                static_cast<int>(val.at("x").as_int64()),
                static_cast<int>(val.at("y").as_int64())
            },
            Size{
                static_cast<int>(val.at("w").as_int64()),
                static_cast<int>(val.at("h").as_int64())
            }
        }
    };
}

Office tag_invoke(json::value_to_tag<Office>&, const json::value& val) {
    return Office {
        Office::Id{std::string{val.at("id").as_string()}},
        Point{
            static_cast<int>(val.at("x").as_int64()),
            static_cast<int>(val.at("y").as_int64())
        },
        Offset{
            static_cast<int>(val.at("offsetX").as_int64()),
            static_cast<int>(val.at("offsetY").as_int64())
        }
    };
}

MapSettings tag_invoke(json::value_to_tag<MapSettings>&, const json::value& val) {
    Map game_map {
        Map::Id{std::string{val.at("id").as_string()}},
        std::string{val.at("name").as_string()}
    };

    for (const auto& road : json::value_to<std::vector<Road>>(val.at("roads"))) {
        game_map.AddRoad(road);
    }
    for (const auto& building : json::value_to<std::vector<Building>>(val.at("buildings"))) {
        game_map.AddBuilding(building);
    }
    for (const auto& office : json::value_to<std::vector<Office>>(val.at("offices"))) {
        game_map.AddOffice(office);
    }

    loot::MapLootTypes loots;
    auto loots_obj = val.at("lootTypes").as_array();
    for (const auto& loot : loots_obj) {
        loots.emplace_back(json::value_to<loot::LootType>(loot));
    }

    std::optional<double> speed = std::nullopt;
    if (val.as_object().contains("dogSpeed")) {
        speed = val.at("dogSpeed").as_double();
    }

    std::optional<size_t> bag_size = std::nullopt;
    if (val.as_object().contains("defaultBagCapacity")) {
        bag_size = val.at("defaultBagCapacity").as_int64();
    }

    return {
        game_map,
        loots,
        speed,
        bag_size
    };
}


void tag_invoke(json::value_from_tag, json::value& val, const Road& road) {
    if (road.IsHorizontal()) {
        val = {
            { "x0", road.GetStart().x },
            { "y0", road.GetStart().y },
            { "x1", road.GetEnd().x }
            /* y1 == y0 */
        };
    } else {
        val = {
            { "x0", road.GetStart().x },
            { "y0", road.GetStart().y },
            /* x1 == x0 */
            { "y1", road.GetEnd().y }
        };
    }
}

void tag_invoke(json::value_from_tag, json::value& val, const Building& building) {
    val = {
        {"x", building.GetBounds().position.x},
        {"y", building.GetBounds().position.y},
        {"w", building.GetBounds().size.width},
        {"h", building.GetBounds().size.height}
    };
}

void tag_invoke(json::value_from_tag, json::value& val, const Office& office) {
    val = {
        {"id", *office.GetId()},
        {"x", office.GetPosition().x},
        {"y", office.GetPosition().y},
        {"offsetX", office.GetOffset().dx},
        {"offsetY", office.GetOffset().dy}
    };
}

void tag_invoke(json::value_from_tag, json::value& val, const Map& map) {
    val = {
        {"id", *map.GetId()},
        {"name", map.GetName()},
        {"roads", map.GetRoads()},
        {"buildings", map.GetBuildings()},
        {"offices", map.GetOffices()}
    };
}

void tag_invoke(json::value_from_tag, json::value& val, const MapShortView& map_view) {
    val = {
        {"id", *map_view.GetId()},
        {"name", map_view.GetName()}
    };
}

void tag_invoke(json::value_from_tag, json::value& val, const std::pair<Map, loot::MapLootTypes>& pair) {
    val = {
        {"id", *pair.first.GetId()},
        {"name", pair.first.GetName()},
        {"roads", pair.first.GetRoads()},
        {"buildings", pair.first.GetBuildings()},
        {"offices", pair.first.GetOffices()},
        {"lootTypes", pair.second}
    };
}

void tag_invoke(json::value_from_tag, json::value &val, const Point2D &pos) {
    val = {pos.x, pos.y};
}

void tag_invoke(json::value_from_tag, json::value &val, const Vec2D &speed) {
    val = {speed.dx, speed.dy};
}

void tag_invoke(json::value_from_tag, json::value &val, const Direction &direction) {
    switch (direction) {
        case Direction::NORTH:
            val = "U";
            break;
        case Direction::SOUTH:
            val = "D";
            break;
        case Direction::WEST:
            val = "L";
            break;
        case Direction::EAST:
            val = "R";
            break;
        case Direction::NONE:
            val = "";
            break;
        default:
            throw std::runtime_error("Unknown direction");
    }
}

void tag_invoke(json::value_from_tag, json::value& val, const CargoItem& cargo_item) {
    val = {
        {"id", cargo_item.id},
        {"type", cargo_item.type}
    };
}

void tag_invoke(json::value_from_tag, json::value &val, const Dog &dog) {
    val = {
        {"pos", dog.GetPosition()},
        {"speed", dog.GetSpeed()},
        {"dir", dog.GetDirection()},
        {"bag", dog.GetBagContent()}, // TODO: check where this is used
        {"score", dog.GetScore()}
    };
}

void tag_invoke(json::value_from_tag, json::value& val, const LootItem& loot_item) {
    val = {
        {"type", loot_item.type},
        {"pos", loot_item.pos}
    };
}

void tag_invoke(json::value_from_tag, json::value& val, const GameState& game_state) {
    val = {
        {"players", game_state.actors},
        {"lostObjects", game_state.loots},
    };
}

} // namespace model
