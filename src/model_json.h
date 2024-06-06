#ifndef GAME_SERVER_MODEL_JSON_H
#define GAME_SERVER_MODEL_JSON_H

#include "loot.h"

namespace model {

struct MapSettings {
    Map map;
    loot::MapLootTypes loots;
    std::optional<double> speed;
    std::optional<size_t> bag_size = 3;
};

namespace json = boost::json;

Road tag_invoke(json::value_to_tag <Road> &, const json::value &val);
Building tag_invoke(json::value_to_tag<Building>&, const json::value& val);
Office tag_invoke(json::value_to_tag<Office>&, const json::value& val);

MapSettings tag_invoke(json::value_to_tag<MapSettings>&, const json::value& val);

void tag_invoke(json::value_from_tag, json::value& val, const Road& road);
void tag_invoke(json::value_from_tag, json::value& val, const Building& building);
void tag_invoke(json::value_from_tag, json::value& val, const Office& office);
void tag_invoke(json::value_from_tag, json::value& val, const Map& map);
void tag_invoke(json::value_from_tag, json::value& val, const MapShortView& map_view);
void tag_invoke(json::value_from_tag, json::value& val, const std::pair<Map, loot::MapLootTypes>& map_loots);

void tag_invoke(json::value_from_tag, json::value& val, const Point2D& pos);
void tag_invoke(json::value_from_tag, json::value& val, const Vec2D& speed);
void tag_invoke(json::value_from_tag, json::value& val, const Direction& direction);
void tag_invoke(json::value_from_tag, json::value& val, const CargoItem& cargo_item);
void tag_invoke(json::value_from_tag, json::value& val, const Dog& dog);

void tag_invoke(json::value_from_tag, json::value& val, const LootItem& loot_item);

void tag_invoke(json::value_from_tag, json::value& val, const GameState& game_state);

}  // namespace model

#endif //GAME_SERVER_MODEL_JSON_H
