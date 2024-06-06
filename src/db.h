#ifndef GAME_SERVER_DB_H
#define GAME_SERVER_DB_H

#include <string>

#include "connection_pool.h"
#include "domain.h"

namespace db {

class RecordDB {
public:
    explicit RecordDB(const std::string& db_url);
    void Save(const domain::RetiredDog& dog);
    domain::Retirees GetRetiredDogs(std::optional<int> start, std::optional<int> max_size);
private:
    connection_pool::ConnectionPool conn_pool_;
};

} // namespace db

#endif //GAME_SERVER_DB_H
