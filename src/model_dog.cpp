#include "model_dog.h"
#include "model_geometry.h"

namespace model {

Dog::Id::ValueType Dog::GetIdValue() const {
    return *id_;
}

std::string Dog::GetName() const {
    return name_;
}

model::Point2D Dog::GetPosition() const {
    return position_;
}

void Dog::SetPosition(const model::Point2D& position) {
    position_ = position;
}

model::Vec2D Dog::GetSpeed() const {
    return speed_;
}

void Dog::SetSpeed(const model::Vec2D& speed) {
    speed_ = speed;
}

Direction Dog::GetDirection() const {
    return direction_;
}

void Dog::SetDirection(const Direction& direction) {
    direction_ = direction;
}

size_t Dog::GetBagCapacity() const {
    return bag_cap_;
}

bool Dog::IsBagFull() const {
    return bag_.size() >= bag_cap_;
}

bool Dog::PutToBag(const CargoItem& item) {
    if (IsBagFull()) {
        return false;
    }
    bag_.push_back(item);
    return true;
}

size_t Dog::EmptyBag() {
    auto res = bag_.size();
    bag_.clear();

    return res;
}

std::vector<CargoItem> Dog::GetBagContent() const {
    return bag_;
}

unsigned Dog::GetScore() const {
    return score_;
}

void Dog::AddScore(unsigned points) {
    score_ += points;
}

std::chrono::milliseconds Dog::GetNonactiveTime() const {
    return nonactive_time;
}

void Dog::AddNonactiveTime(std::chrono::milliseconds time) {
    nonactive_time += time;
}

void Dog::ResetNonactiveTime() {
    nonactive_time = std::chrono::milliseconds(0);
}

void Dog::AddPlayTime(std::chrono::milliseconds time) {
    play_time += time;
}

std::chrono::milliseconds Dog::GetPlayTime() const {
    return play_time;
}

} // namespace model
