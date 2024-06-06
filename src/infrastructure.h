#ifndef INFRASTRUCTURE_H
#define INFRASTRUCTURE_H

#include <chrono>
#include <memory>
#include <utility>

#include "app.h"

namespace infrastructure {

class Autosaver {
public:
    explicit Autosaver(app::App &app, std::string state_file, std::chrono::milliseconds period)
            : app_(app)
            , state_file_(std::move(state_file))
            , save_period_(period) {
    }
    void Save();
    void OnTick(std::chrono::milliseconds delta);
    void Restore();
private:
    app::App &app_;
    std::string state_file_;
    std::chrono::milliseconds save_period_;
    std::chrono::milliseconds elapsed_ = std::chrono::milliseconds(0);
};

} // namespace infrastructure

#endif  // INFRASTRUCTURE_H