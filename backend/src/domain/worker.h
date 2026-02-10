#pragma once

#include "pricelist.h"
#include "tarif.h"
#include "trunk.h"
#include "call_statistics.h"
#include "../ui/view.h"

namespace domain {

class Worker {
  public:
    virtual void AddPricelist(const domain::Pricelist& pricelist) = 0;
    virtual void UpdatePricelist(const domain::Pricelist& pricelist, int id) = 0;

    virtual void AddTarif(const domain::Tarif& tarif) = 0;
    virtual void UpdateTarif(const domain::Tarif& tarif, int id) = 0;

    virtual void AddTrunk(const domain::Trunk& trunk) = 0;
    virtual void UpdateTrunk(const domain::Trunk& trunk, int id) = 0;

    virtual void AddCallStatistics(const domain::CallStatistics& call_stat) = 0;

  protected:
    virtual ~Worker() = default;
};

} // namespace domain
