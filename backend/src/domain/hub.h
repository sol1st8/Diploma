#pragma once

#include "../ui/view.h"

#include <memory>

namespace domain {

class Worker;

class Hub {
  public:
    Hub(int id, std::string name, std::string location, bool is_active)
        : id_(id)
        , name_(std::move(name))
        , location_(std::move(location))
        , is_active_(is_active) {}

    int GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const std::string& GetLocation() const noexcept {
        return location_;
    }

    bool GetIsActive() const noexcept {
        return is_active_;
    }

  private:
    int id_;
    std::string name_;
    std::string location_;
    bool is_active_;
};

class HubRepository {
  public:
    virtual std::vector<ui::detail::HubInfo> Get() const = 0;

    virtual std::shared_ptr<domain::Worker> GetWorker() const = 0;

  protected:
    ~HubRepository() = default;
};

} // namespace domain
