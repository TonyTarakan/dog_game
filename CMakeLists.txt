cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# To generate uml diagram
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# TODO: fix conan 1->2 migration
include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)

find_package(Boost)
find_package(Catch2)
find_package(libpqxx)

# boost.beast будет использовать std::string_view вместо boost::string_view
add_compile_definitions(BOOST_BEAST_USE_STD_STRING_VIEW)

add_library(game_model_lib STATIC
	src/db.cpp
	src/db.h
	src/domain.cpp
	src/domain.h
	src/boost_json.cpp
	src/sdk.h
	src/geom.h
	src/serialization.h
	src/infrastructure.cpp
	src/infrastructure.h
	src/loot.cpp
	src/loot.h
	src/model.cpp
	src/model.h
	src/model_dog.cpp
	src/model_dog.h
	src/model_geometry.cpp
	src/model_geometry.h
	src/model_json.cpp
	src/model_json.h
	src/tagged.h
)
target_link_libraries(game_model_lib PUBLIC boost::boost libpqxx::pqxx)

add_library(collision_detection_lib STATIC
	src/collision_detector.h
	src/collision_detector.cpp
)

add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/handler_serializer.cpp
	src/handler_serializer.h
	src/json_loader.cpp
	src/json_loader.h
	src/request_handler.cpp
	src/request_handler.h
	src/logger.cpp
	src/logger.h
	src/app.cpp
	src/app.h
	src/ticker.h
)
target_link_libraries(game_server PRIVATE boost::boost game_model_lib collision_detection_lib)

#
# Tests
#
add_executable(unit_tests
	tests/model_tests.cpp
	tests/loot_generator_tests.cpp
	tests/collision_detector_tests.cpp
	tests/state-serialization-tests.cpp
)
target_link_libraries(unit_tests PRIVATE Catch2::Catch2WithMain boost::boost game_model_lib collision_detection_lib)