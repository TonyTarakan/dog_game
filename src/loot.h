#ifndef LOOT_H
#define LOOT_H

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <boost/json.hpp>

namespace loot {

namespace json = boost::json;

struct LootType {
    std::string name;
    std::string file;
    std::string type;
    std::optional<long> rotation = std::nullopt;
    std::optional<std::string> color = std::nullopt;
    std::optional<double> scale = std::nullopt;
    std::optional<int> value = std::nullopt;
};

LootType tag_invoke(json::value_to_tag<LootType>&, const json::value &val);

void tag_invoke(json::value_from_tag, json::value& val, const LootType& obj);

using MapLootTypes = std::vector<LootType>;

/*
 *  Генератор трофеев
 */
class LootGenerator {
public:
    using RandomGenerator = std::function<double()>;
    using TimeInterval = std::chrono::milliseconds;

    /*
     * base_interval - базовый отрезок времени > 0
     * probability - вероятность появления трофея в течение базового интервала времени
     * random_generator - генератор псевдослучайных чисел в диапазоне от [0 до 1]
     */
    LootGenerator(TimeInterval base_interval, double probability,
                  RandomGenerator random_gen = DefaultGenerator)
        : base_interval_{base_interval}
        , probability_{probability}
        , random_generator_{std::move(random_gen)} {
    }

    /*
     * Возвращает количество трофеев, которые должны появиться на карте спустя
     * заданный промежуток времени.
     * Количество трофеев, появляющихся на карте не превышает количество мародёров.
     *
     * time_delta - отрезок времени, прошедший с момента предыдущего вызова Generate
     * loot_count - количество трофеев на карте до вызова Generate
     * looter_count - количество мародёров на карте
     */
    unsigned Generate(TimeInterval time_delta, unsigned loot_count, unsigned looter_count);

private:
    static double DefaultGenerator() noexcept {
        return 1.0;
    };
    TimeInterval base_interval_;
    double probability_;
    TimeInterval time_without_loot_{};
    RandomGenerator random_generator_;
};

}  // namespace loot

#endif  // LOOT_H