#pragma once

#include "../ui/view.h"

#include <memory>

namespace domain {

class Worker;

class Server {
  public:
    Server(int id, int hub_id, std::string name, bool is_active)
        : id_(id)
        , hub_id_(hub_id)
        , name_(std::move(name))
        , is_active_(is_active) {}

    int GetId() const noexcept {
        return id_;
    }

    int GetHubId() const noexcept {
        return hub_id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    bool GetIsActive() const noexcept {
        return is_active_;
    }

  private:
    int id_;
    int hub_id_;
    std::string name_;
    bool is_active_;
};

class ServerRepository {
  public:
    virtual std::vector<ui::detail::ServerInfo> Get() const = 0;

    virtual std::shared_ptr<domain::Worker> GetWorker() const = 0;

  protected:
    ~ServerRepository() = default;
};

} // namespace domain
