#pragma once

#include "use_cases.h"
#include "../domain/hub_fwd.h"
#include "../domain/server_fwd.h"
#include "../domain/nas_ip_fwd.h"
#include "../domain/trunk_fwd.h"
#include "../domain/pricelist_fwd.h"
#include "../domain/tarif_fwd.h"
#include "../domain/call_statistics_fwd.h"

namespace app {

class UseCasesImpl : public UseCases {
  public:
    explicit UseCasesImpl(domain::HubRepository& hubs,
                          domain::ServerRepository& servers,
                          domain::NasIpRepository& nas_ips,
                          domain::TrunkRepository& trunks,
                          domain::PricelistRepository& pricelists,
                          domain::TarifRepository& tarifs,
                          domain::CallStatisticsRepository& call_statistics);

    std::vector<ui::detail::HubInfo> GetHubs() const override;
    std::vector<ui::detail::ServerInfo> GetServers() const override;
    std::vector<ui::detail::NasIpInfo> GetNasIps() const override;
    std::vector<ui::detail::TrunkInfo> GetTrunks() const override;
    std::vector<ui::detail::PricelistInfo> GetPricelists() const override;
    std::vector<ui::detail::TarifInfo> GetTarifs() const override;
    std::vector<ui::detail::CallStatisticsInfo> GetCallStatistics() const override;

    void AddPricelist(const ui::detail::PricelistInfo& pricelist) override;
    void UpdatePricelist(const ui::detail::PricelistInfo& pricelist, int id) override;

    void AddTarif(const ui::detail::TarifInfo& tarif) override;
    void UpdateTarif(const ui::detail::TarifInfo& tarif, int id) override;

    void AddTrunk(const ui::detail::TrunkInfo& trunk) override;
    void UpdateTrunk(const ui::detail::TrunkInfo& trunk, int id) override;

    void AddCallStatistics(const ui::detail::CallStatisticsInfo& call_stat) override;

  private:
    domain::HubRepository& hubs_;
    domain::ServerRepository& servers_;
    domain::NasIpRepository& nas_ips_;
    domain::TrunkRepository& trunks_;
    domain::PricelistRepository& pricelists_;
    domain::TarifRepository& tarifs_;
    domain::CallStatisticsRepository& call_statistics_;
};

} // namespace app
