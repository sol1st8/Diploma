#pragma once

#include "../ui/view.h"

#include <memory>

namespace domain {

class Worker;

class Tarif {
  public:
    Tarif(int id, std::string name, int pricelist_id, int markup_percent, int free_minutes)
        : id_(id)
        , name_(std::move(name))
        , pricelist_id_(pricelist_id)
        , markup_percent_(markup_percent)
        , free_minutes_(free_minutes) {}

    int GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    int GetPricelistId() const noexcept {
        return pricelist_id_;
    }

    int GetMarkupPercent() const noexcept {
        return markup_percent_;
    }

    int GetFreeMinutes() const noexcept {
        return free_minutes_;
    }

  private:
    int id_;
    std::string name_;
    int pricelist_id_;
    int markup_percent_;
    int free_minutes_;
};

class TarifRepository {
  public:
    virtual std::vector<ui::detail::TarifInfo> Get() const = 0;

    virtual std::shared_ptr<domain::Worker> GetWorker() const = 0;

  protected:
    ~TarifRepository() = default;
};

} // namespace domain
