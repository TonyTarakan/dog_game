#include <fstream>
#include <filesystem>
#include <iostream>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "infrastructure.h"
#include "serialization.h"
#include "logger.h"

namespace infrastructure {
using InArchive = boost::archive::text_iarchive;
using OutArchive = boost::archive::text_oarchive;

void Autosaver::Restore() {
    if (!std::filesystem::exists(state_file_)) {
        logger::Logger::log_json("autosave not found", {{"file", state_file_}});
        return;
    }
    try {
        std::ifstream archive_{state_file_};
        InArchive input_archive{archive_ };
        serialization::AppRepr repr;
        input_archive >> repr;
        repr.Restore(app_);
        logger::Logger::log_json("autosave restored", {{"file", state_file_}});
    }
    catch (const std::exception& ex) {
        logger::Logger::log_json("restore error", {{"error", ex.what()}});
    }
}

void Autosaver::Save() {
    try {
        std::ofstream archive_{state_file_};
        OutArchive output_archive{archive_};
        output_archive << serialization::AppRepr{app_};
        logger::Logger::log_json("state saved", {{"file", state_file_}});
    }
    catch (const std::exception& ex) {
        logger::Logger::log_json("autosave error", {{"error", ex.what()}});
        throw;
    }
    catch (...) {
        std::cout << "unknown error" << std::endl;
        throw;
    }
}

void Autosaver::OnTick(std::chrono::milliseconds delta) {
    elapsed_ += delta;
    if (save_period_ > std::chrono::milliseconds{0}
        && elapsed_ >= save_period_) {
        Save();
        elapsed_ = std::chrono::milliseconds{0};
    }
}

} // namespace infrastructure
