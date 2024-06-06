#include <algorithm>
#include <cmath>
#include "loot.h"

namespace loot {

LootType tag_invoke(json::value_to_tag<LootType>&, const json::value &val) {
    auto obj = val.as_object();
    LootType loot;
    loot.name = obj.at("name").as_string().c_str();
    loot.file = obj.at("file").as_string().c_str();
    loot.type = obj.at("type").as_string().c_str();
    if (obj.contains("rotation")) {
        loot.rotation = obj.at("rotation").as_int64();
    }
    if (obj.contains("color")) {
        loot.color = obj.at("color").as_string().c_str();
    }
    if (obj.contains("scale")) {
        loot.scale = obj.at("scale").as_double();
    }
    if (obj.contains("value")) {
        loot.value = obj.at("value").as_int64();
    }
    return loot;
}

void tag_invoke(json::value_from_tag, json::value& val, const LootType& loot) {
    json::value obj = {
        {"name", loot.name},
        {"file", loot.file},
        {"type", loot.type},
    };
    if (loot.rotation) {
        obj.as_object()["rotation"] = loot.rotation.value();
    }
    if (loot.color) {
        obj.as_object()["color"] = loot.color.value();
    }
    if (loot.scale) {
        obj.as_object()["scale"] = loot.scale.value();
    }
    if (loot.value) {
        obj.as_object()["value"] = loot.value.value();
    }
    val = std::move(obj);
}

unsigned LootGenerator::Generate(TimeInterval time_delta, unsigned loot_count, unsigned looter_count) {
    time_without_loot_ += time_delta;
    const unsigned loot_shortage = loot_count > looter_count ? 0u : looter_count - loot_count;
    const double ratio = std::chrono::duration<double>{time_without_loot_} / base_interval_;
    const double probability
        = std::clamp((1.0 - std::pow(1.0 - probability_, ratio)) * random_generator_(), 0.0, 1.0);
    const auto generated_loot = static_cast<unsigned>(std::round(loot_shortage * probability));
    if (generated_loot > 0) {
        time_without_loot_ = {};
    }
    return generated_loot;
}

} // namespace loot
