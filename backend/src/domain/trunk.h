#pragma once

#include "../ui/view.h"

#include <memory>

namespace domain {

class Worker;

class Trunk {
  public:
    Trunk(int id, int server_id, std::string name, int capacity, double cost_per_channel)
        : id_(id)
        , server_id_(server_id)
        , name_(std::move(name))
        , capacity_(capacity)
        , cost_per_channel_(cost_per_channel) {}

    int GetId() const noexcept {
        return id_;
    }

    int GetServerId() const noexcept {
        return server_id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    int GetCapacity() const noexcept {
        return capacity_;
    }

    double GetCostPerChannel() const noexcept {
        return cost_per_channel_;
    }

  private:
    int id_;
    int server_id_;
    std::string name_;
    int capacity_;
    double cost_per_channel_;
};

class TrunkRepository {
  public:
    virtual std::vector<ui::detail::TrunkInfo> Get() const = 0;

    virtual std::shared_ptr<domain::Worker> GetWorker() const = 0;

  protected:
    ~TrunkRepository() = default;
};

} // namespace domain
