#pragma once

#include "../ui/view.h"

#include <memory>

namespace domain {

class Worker;

class Pricelist {
  public:
    Pricelist(int id, std::string name, std::string currency, double rate_per_minute, bool is_active)
        : id_(id)
        , name_(std::move(name))
        , currency_(std::move(currency))
        , rate_per_minute_(rate_per_minute)
        , is_active_(is_active) {}

    int GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const std::string& GetCurrency() const noexcept {
        return currency_;
    }

    double GetRatePerMinute() const noexcept {
        return rate_per_minute_;
    }

    bool GetIsActive() const noexcept {
        return is_active_;
    }

  private:
    int id_;
    std::string name_;
    std::string currency_;
    double rate_per_minute_;
    bool is_active_;
};

class PricelistRepository {
  public:
    virtual std::vector<ui::detail::PricelistInfo> Get() const = 0;

    virtual std::shared_ptr<domain::Worker> GetWorker() const = 0;

  protected:
    ~PricelistRepository() = default;
};

} // namespace domain
