#pragma once

#include "../ui/view.h"

#include <memory>

namespace domain {

class Worker;

class CallStatistics {
  public:
    CallStatistics(int64_t id, std::string call_id, int trunk_id, int tarif_id, 
                   int duration_seconds, double cost, std::string call_time)
        : id_(id)
        , call_id_(std::move(call_id))
        , trunk_id_(trunk_id)
        , tarif_id_(tarif_id)
        , duration_seconds_(duration_seconds)
        , cost_(cost)
        , call_time_(std::move(call_time)) {}

    int64_t GetId() const noexcept {
        return id_;
    }

    const std::string& GetCallId() const noexcept {
        return call_id_;
    }

    int GetTrunkId() const noexcept {
        return trunk_id_;
    }

    int GetTarifId() const noexcept {
        return tarif_id_;
    }

    int GetDurationSeconds() const noexcept {
        return duration_seconds_;
    }

    double GetCost() const noexcept {
        return cost_;
    }

    const std::string& GetCallTime() const noexcept {
        return call_time_;
    }

  private:
    int64_t id_;
    std::string call_id_;
    int trunk_id_;
    int tarif_id_;
    int duration_seconds_;
    double cost_;
    std::string call_time_;
};

class CallStatisticsRepository {
  public:
    virtual std::vector<ui::detail::CallStatisticsInfo> Get() const = 0;

    virtual std::shared_ptr<domain::Worker> GetWorker() const = 0;

  protected:
    ~CallStatisticsRepository() = default;
};

} // namespace domain
