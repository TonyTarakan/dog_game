#include "db.h"

namespace db {

using pqxx::operator"" _zv;

RecordDB::RecordDB(const std::string& db_url)
        : conn_pool_{
        1, //std::thread::hardware_concurrency(), // TODO: fix tests
        [&db_url](){
            return std::make_shared<pqxx::connection>(db_url);
        }} {
    auto conn = conn_pool_.GetConnection();
    pqxx::work work{*conn};
    work.exec(
        R"(
        CREATE TABLE IF NOT EXISTS retired_players (
        id SERIAL PRIMARY KEY,
        name varchar(100) NOT NULL,
        score integer,
        play_time_ms integer))"_zv
    );
    work.commit();
}

void RecordDB::Save(const domain::RetiredDog& dog) {
    auto conn = conn_pool_.GetConnection();
    pqxx::work work{*conn};
    auto response = work.exec_params(
            R"(INSERT INTO retired_players (id, name, score, play_time_ms) VALUES (DEFAULT, $1, $2, $3);)"_zv,
            dog.name,
            dog.score,
            dog.game_duration.count()
    );
    work.commit();
}

domain::Retirees RecordDB::GetRetiredDogs(std::optional<int> start, std::optional<int> max_size) {
    auto conn = conn_pool_.GetConnection();
    pqxx::work work{*conn};
    std::string q = "SELECT name, score, play_time_ms FROM retired_players ORDER BY score DESC, play_time_ms, name";
    if (max_size.has_value()) {
        q += " LIMIT " + std::to_string(*max_size);
    }
    if (start.has_value()) {
        q += " OFFSET " + std::to_string(*start);
    }
    q += ";";

    std::vector<domain::RetiredDog> result;
    for (auto& [name, score, time] : work.query<std::string, size_t, size_t>(q)) {
        domain::RetiredDog ret;
        ret.name = name;
        ret.score = score;
        ret.game_duration = std::chrono::milliseconds(time);
        result.push_back(ret);
    }
    return result;
}

}