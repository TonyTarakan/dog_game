#ifndef JSON_LOADER_H
#define JSON_LOADER_H

#include <filesystem>
#include <boost/json/value_to.hpp>

#include "model.h"

namespace json_loader {

std::shared_ptr<model::Game> LoadGame(const std::filesystem::path& json_path);

}  // namespace json_loader

#endif  // JSON_LOADER_H