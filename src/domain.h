#ifndef GAME_SERVER_DB_DOMAIN_H
#define GAME_SERVER_DB_DOMAIN_H

#include <chrono>
#include <string>
#include <vector>

#include <boost/json.hpp>

namespace domain {

struct RetiredDog {
    std::string name;
    unsigned score;
    std::chrono::milliseconds game_duration;
};

using Retirees = std::vector<domain::RetiredDog>;

void tag_invoke(boost::json::value_from_tag, boost::json::value& val, const RetiredDog& dog);

}  // namespace domain

#endif  // GAME_SERVER_DB_DOMAIN_H