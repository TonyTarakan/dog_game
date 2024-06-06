#define _USE_MATH_DEFINES

#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <catch2/catch_approx.hpp>

#include "../src/collision_detector.h"

using namespace collision_detector;
using namespace model;

// тесты для функции collision_detector::FindGatherEvents
namespace Catch {

}  // namespace Catch

namespace gather_tests {

class Provider : public collision_detector::ItemGathererProvider
{
public:
    explicit Provider(std::vector<collision_detector::Item> items,
             std::vector<collision_detector::Gatherer> gatherers)
            : items_(std::move(items)), gatherers_(std::move(gatherers)) {
    }

    [[nodiscard]] size_t ItemsCount() const override {
        return items_.size();
    }
    [[nodiscard]] collision_detector::Item GetItem(size_t idx) const override {
        return items_.at(idx);
    }
    [[nodiscard]] size_t GatherersCount() const override {
        return gatherers_.size();
    }
    [[nodiscard]] collision_detector::Gatherer GetGatherer(size_t idx) const override {
        return gatherers_.at(idx);
    }

private:
    std::vector<collision_detector::Item> items_;
    std::vector<collision_detector::Gatherer> gatherers_;
};

}  // namespace gather_tests


SCENARIO("Collision detector", "[collision_detector]") {

    using namespace collision_detector;
    using namespace model;
    using Catch::Matchers::Contains;
    using Catch::Matchers::IsEmpty;
    using Catch::Matchers::Predicate;
    using Catch::Matchers::WithinAbs;

    GIVEN("An empty items and gatherers") {
        std::vector<Item> items{};
        std::vector<Gatherer> gatherers{};
        gather_tests::Provider provider(items, gatherers);
        THEN("provider is empty") {
            CHECK(provider.ItemsCount() == 0);
            CHECK(provider.GatherersCount() == 0);
            CHECK_THROWS(provider.GetItem(0));
            CHECK_THROWS(provider.GetGatherer(0));
        }
        AND_THEN("no events are found") {
            CHECK_THAT(FindGatherEvents(provider), IsEmpty());
        }
    }

    GIVEN("An empty items and single gatherer") {
        std::vector<Item> items{};
        constexpr double gw = 0.5; // gatherer width
        std::vector<Gatherer> gatherers{ Gatherer{{0, 0}, {0, 0}, gw} };
        gather_tests::Provider provider(items, gatherers);
        THEN("provider contains one gatherer") {
            CHECK(provider.ItemsCount() == 0);
            CHECK(provider.GatherersCount() == 1);
            CHECK_THROWS(provider.GetItem(0));
            CHECK_NOTHROW(provider.GetGatherer(0));
        }
        AND_THEN("no events are found") {
            CHECK_THAT(FindGatherEvents(provider), IsEmpty());
        }
    }

    GIVEN("One non-moving gatherer and one far away item") {
        constexpr double gw = 0.6; // gatherer width
        constexpr double iw = 0.5; // item width
        Point2D pos_zero{0.0, 0.0};
        Point2D pos_far{100.0, 0.0};

        std::vector<Item> items{ {1, pos_far, iw} };
        std::vector<Gatherer> gatherers{ Gatherer{pos_zero, pos_zero, gw} };
        gather_tests::Provider provider(items, gatherers);
        THEN("provider contains one item and one gatherer") {
            CHECK(provider.ItemsCount() == 1);
            CHECK(provider.GatherersCount() == 1);
            CHECK_NOTHROW(provider.GetItem(0));
            CHECK_NOTHROW(provider.GetGatherer(0));
            CHECK_THAT(provider.GetItem(0), Predicate<Item>([&](const Item& item) {
                return item.position == pos_far && item.width == iw;
            }));
            CHECK_THAT(provider.GetGatherer(0), Predicate<Gatherer>([&](const Gatherer& gatherer) {
                return gatherer.start_pos == pos_zero && gatherer.end_pos == pos_zero && gatherer.width == gw;
            }));
        }
        AND_THEN("no events are found") {
            CHECK_THAT(FindGatherEvents(provider), IsEmpty());
        }
    }

    GIVEN("One non-moving gatherer and one close item") {
        constexpr double gw = 0.6; // gatherer width
        constexpr double iw = 0.5; // item width
        Point2D pos_zero{0.0, 0.0};
        Point2D pos_close{0.5, 0.0};

        std::vector<Item> items{ {1, pos_close, iw} };
        std::vector<Gatherer> gatherers{ Gatherer{pos_zero, pos_zero, gw} };
        gather_tests::Provider provider(items, gatherers);
        AND_THEN("no events are found") {
            CHECK_THAT(FindGatherEvents(provider), IsEmpty());
        }
    }

    GIVEN("One moving gatherer and one item, horizontal movement") {
        constexpr double gw = 0.6; // gatherer width
        constexpr double iw = 0.5; // item width
        Point2D pos_gather_start{0.0, 0.0};
        Point2D pos_gather_end{0.5, 0.0};
        Point2D item_pos{0.5, 0.0};

        std::vector<Item> items{ {1, item_pos, iw} };
        std::vector<Gatherer> gatherers{ Gatherer{pos_gather_start, pos_gather_end, gw} };
        gather_tests::Provider provider(items, gatherers);
        THEN("Gather event is found") {
            CHECK(FindGatherEvents(provider).size() == 1);
        }
    }

    GIVEN("One moving gatherer and one item, vertical movement") {
        constexpr double gw = 0.6; // gatherer width
        constexpr double iw = 0.5; // item width
        Point2D pos_gather_start{0.0, 0.0};
        Point2D pos_gather_end{0.0, 0.5};
        Point2D item_pos{0.0, 0.5};

        std::vector<Item> items{ {1, item_pos, iw} };
        std::vector<Gatherer> gatherers{ Gatherer{pos_gather_start, pos_gather_end, gw} };
        gather_tests::Provider provider(items, gatherers);
        THEN("Gather event is found") {
            CHECK(FindGatherEvents(provider).size() == 1);
        }
    }

    GIVEN("One moving gatherer and one item, diagonal movement") {
        constexpr double gw = 0.6; // gatherer width
        constexpr double iw = 0.5; // item width
        Point2D pos_gather_start{0.0, 0.0};
        Point2D pos_gather_end{0.5, 0.5};
        Point2D item_pos{0.5, 0.5};

        std::vector<Item> items{ {1, item_pos, iw} };
        std::vector<Gatherer> gatherers{ Gatherer{pos_gather_start, pos_gather_end, gw} };
        gather_tests::Provider provider(items, gatherers);
        THEN("Gather event is found") {
            CHECK(FindGatherEvents(provider).size() == 1);
        }
    }

    GIVEN("One moving gatherer and one item, drive by movement") {
        constexpr double gw = 0.6; // gatherer width
        constexpr double iw = 0.5; // item width
        Point2D pos_gather_start{0.0, 0.0};
        Point2D pos_gather_end{10.0, 0.0};
        Point2D item_pos{5.0, 1.0};

        std::vector<Item> items{ {1, item_pos, iw} };
        std::vector<Gatherer> gatherers{ Gatherer{pos_gather_start, pos_gather_end, gw} };
        gather_tests::Provider provider(items, gatherers);
        THEN("Gather event is found") {
            CHECK(FindGatherEvents(provider).size() == 1);
        }
    }

    GIVEN("One moving gatherer and one item, touch movement") {
        constexpr double gw = 0.6; // gatherer width
        constexpr double iw = 0.5; // item width
        Point2D pos_gather_start{0.0, 0.0};
        Point2D pos_gather_end{10.0, 0.0};
        Point2D item_pos{5.0, 1.1};

        std::vector<Item> items{ {1, item_pos, iw} };
        std::vector<Gatherer> gatherers{ Gatherer{pos_gather_start, pos_gather_end, gw} };
        gather_tests::Provider provider(items, gatherers);
        THEN("Gather event is not found") {
            CHECK(FindGatherEvents(provider).empty());
        }
    }

    GIVEN("One moving gatherer and few items") {
        constexpr double gw = 0.6; // gatherer width
        constexpr double iw = 0.5; // item width
        Point2D pos_gather_start{0.0, 0.0};
        Point2D pos_gather_end{10.0, 0.0};
        Point2D item_pos_0{0.0, 0.0};
        Point2D item_pos_1{5.0, 1.0};
        Point2D item_pos_2{10.0, 2.0};

        std::vector<Item> items{ {0, item_pos_0, iw}, {1, item_pos_1, iw}, {2, item_pos_2, iw} };
        std::vector<Gatherer> gatherers{ Gatherer{pos_gather_start, pos_gather_end, gw} };
        gather_tests::Provider provider(items, gatherers);
        THEN("Gather events are found, items 0 and 1") {
            auto events = FindGatherEvents(provider);
            CHECK(events.size() == 2);
            CHECK(events.at(0).item_id == 0);
            CHECK(events.at(1).item_id == 1);
        }
    }

}
