#ifndef GAME_SERVER_COLLISION_DETECTOR_H
#define GAME_SERVER_COLLISION_DETECTOR_H

#include <algorithm>
#include <vector>

#include "geom.h"

namespace collision_detector {

struct CollectionResult {
    double sq_distance;
    double proj_ratio;
    [[nodiscard]] bool IsCollected(double collect_radius) const {
        return proj_ratio >= 0 && proj_ratio <= 1 && sq_distance <= collect_radius * collect_radius;
    }
};

CollectionResult TryCollectPoint(model::Point2D a, model::Point2D b, model::Point2D c);

struct Item {
    u_int64_t id;
    model::Point2D position;
    double width;
};

struct Gatherer {
    model::Point2D start_pos;
    model::Point2D end_pos;
    double width;
};

class ItemGathererProvider {
protected:
    ~ItemGathererProvider() = default;

public:
    virtual size_t ItemsCount() const = 0;
    virtual Item GetItem(size_t idx) const = 0;
    virtual size_t GatherersCount() const = 0;
    virtual Gatherer GetGatherer(size_t idx) const = 0;
};

class Provider : public ItemGathererProvider
{
public:
    explicit Provider(std::vector<Item> items,
                      std::vector<Gatherer> gatherers)
            : items_(std::move(items)), gatherers_(std::move(gatherers)) {
    }

    [[nodiscard]] size_t ItemsCount() const override {
        return items_.size();
    }
    [[nodiscard]] Item GetItem(size_t idx) const override {
        return items_.at(idx);
    }
    [[nodiscard]] size_t GatherersCount() const override {
        return gatherers_.size();
    }
    [[nodiscard]] Gatherer GetGatherer(size_t idx) const override {
        return gatherers_.at(idx);
    }

private:
    std::vector<Item> items_;
    std::vector<Gatherer> gatherers_;
};

struct GatheringEvent {
    size_t item_id;
    size_t gatherer_id;
    double time;
};

std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider);

}  // namespace collision_detector

#endif  // GAME_SERVER_COLLISION_DETECTOR_H