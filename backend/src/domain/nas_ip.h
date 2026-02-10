#pragma once

#include "../ui/view.h"

#include <memory>

namespace domain {

class Worker;

class NasIp {
  public:
    NasIp(int id, int server_id, std::string ip_address, std::string description)
        : id_(id)
        , server_id_(server_id)
        , ip_address_(std::move(ip_address))
        , description_(std::move(description)) {}

    int GetId() const noexcept {
        return id_;
    }

    int GetServerId() const noexcept {
        return server_id_;
    }

    const std::string& GetIpAddress() const noexcept {
        return ip_address_;
    }

    const std::string& GetDescription() const noexcept {
        return description_;
    }

  private:
    int id_;
    int server_id_;
    std::string ip_address_;
    std::string description_;
};

class NasIpRepository {
  public:
    virtual std::vector<ui::detail::NasIpInfo> Get() const = 0;

    virtual std::shared_ptr<domain::Worker> GetWorker() const = 0;

  protected:
    ~NasIpRepository() = default;
};

} // namespace domain
