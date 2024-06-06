#include <cmath>
#include <catch2/catch_test_macros.hpp>

#include "../src/model_dog.h"
#include "../src/model.h"
#include "../src/loot.h"

using namespace std::literals;

SCENARIO("Game model", "[model]") {
    GIVEN("Test game model") {
        auto game = std::make_shared<model::Game>();
        game->SetDefaultSpeed(1.0f);
        game->SetDefaultBagSize(1);
        auto gen_ptr = std::make_shared<loot::LootGenerator>(1s, 1.0);
        game->SetLootGenerator(gen_ptr);
        REQUIRE(game->GetDefaultSpeed() == 1.0f);

        model::Map map{model::Map::Id{"test_map_0"}, "Test Map 0"};
        model::Road road{{0, 0}, {0, 10}};
        map.AddRoad(road);
        REQUIRE(map.GetRoads().size() == 1);
        REQUIRE(map.GetRoads().at(0) == road);

        auto road_rect = road.GetBounds();
        constexpr double road_half_width = 0.4;
        REQUIRE(road_rect.min_corner.x == -road_half_width);
        REQUIRE(road_rect.min_corner.y == -road_half_width);
        REQUIRE(road_rect.max_corner.x == road_half_width);
        REQUIRE(road_rect.max_corner.y == 10.0 + road_half_width);

        model::Map bad_map{model::Map::Id{"bad_map"}, "Bad Map"};
        REQUIRE_THROWS(game->GetSession(bad_map));

        game->AddMap(map);
        REQUIRE(game->GetMaps().size() == 1);
        REQUIRE(game->FindMap("test_map_0") != nullptr);
        REQUIRE(game->FindMap("bad_map") == nullptr);

        std::vector<loot::LootType> loot_types{
            {"golden_coin", "", ""},
        };
        std::map<model::Map::Id, loot::MapLootTypes> loot{{map.GetId(), loot_types}};
        game->SetLootData(loot);

        auto session = game->GetSession(map);
        REQUIRE(session != nullptr);

        REQUIRE(session->GetDogs().empty());
        REQUIRE(session->GetLoots().empty());

        session->AddDog(0, std::string());
        REQUIRE(session->GetDogs().size() == 1);

        session->Tick(100);
        REQUIRE(session->GetLoots().size() <= 1);
    }
}
