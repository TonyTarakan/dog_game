#include "domain.h"

namespace domain {

constexpr double MS_PER_SECOND = 1000.0;

void tag_invoke(boost::json::value_from_tag, boost::json::value &val, const RetiredDog &dog) {
    val = {
        {"name",     dog.name},
        {"score",    dog.score},
        {"playTime", static_cast<double>(dog.game_duration.count()) / MS_PER_SECOND},
    };
}

}