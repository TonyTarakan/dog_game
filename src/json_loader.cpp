#include <fstream>
#include <iostream>
#include <string>

#include <boost/json.hpp>

#include "json_loader.h"
#include "model_json.h"

namespace json = boost::json;

namespace json_loader {

constexpr int MS_PER_SEC = 1000;

std::shared_ptr<model::Game> LoadGame(const std::filesystem::path& json_path) {

    // TODO: spaghetti code, refactor

    std::ifstream json_file(json_path);
    if (!json_file.is_open()) {
        throw std::runtime_error("Failed to open json file: " + json_path.string());
    }
    std::string json_string((std::istreambuf_iterator<char>(json_file)), std::istreambuf_iterator<char>());

    const auto game_json = json::parse(json_string);

    auto game = std::make_shared<model::Game>();
    game->SetDefaultSpeed(json::value_to<double>(game_json.at("defaultDogSpeed")));
    if (game_json.as_object().contains("defaultBagCapacity")) {
        game->SetDefaultBagSize(json::value_to<size_t>(game_json.at("defaultBagCapacity")));
    }

    auto loot_sec_interval = json::value_to<double>(game_json.at("lootGeneratorConfig").at("period"));
    std::chrono::milliseconds loot_interval(static_cast<int>(loot_sec_interval * MS_PER_SEC));
    auto loot_chance = json::value_to<double>(game_json.at("lootGeneratorConfig").at("probability"));
    auto loot_gen_ptr = std::make_shared<loot::LootGenerator>(loot_interval, loot_chance);
    game->SetLootGenerator(loot_gen_ptr);

    auto retirement_time = json::value_to<double>(game_json.at("dogRetirementTime"));
    game->SetRetirementTime(retirement_time);

    auto map_data = json::value_to<std::vector<model::MapSettings>>(game_json.at("maps"));

    std::map<model::Map::Id, loot::MapLootTypes> loot;
    for (auto& [map, map_loots, speed, bag_size] : map_data) {
        double map_speed = speed ? speed.value() : game->GetDefaultSpeed();
        map.SetSpeed(map_speed);
        size_t map_bag_size = bag_size ? bag_size.value() : game->GetDefaultBagSize();
        map.SetBagSize(map_bag_size);
        game->AddMap(map);
        loot.insert({map.GetId(), map_loots});
    }
    game->SetLootData(loot);

    return game;
}

}  // namespace json_loader
