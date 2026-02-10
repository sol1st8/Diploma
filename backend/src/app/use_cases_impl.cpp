#include "use_cases_impl.h"
#include "../domain/worker.h"
#include "../domain/hub.h"
#include "../domain/server.h"
#include "../domain/nas_ip.h"
#include "../domain/trunk.h"
#include "../domain/pricelist.h"
#include "../domain/tarif.h"
#include "../domain/call_statistics.h"

namespace app {

UseCasesImpl::UseCasesImpl(domain::HubRepository& hubs,
                           domain::ServerRepository& servers,
                           domain::NasIpRepository& nas_ips,
                           domain::TrunkRepository& trunks,
                           domain::PricelistRepository& pricelists,
                           domain::TarifRepository& tarifs,
                           domain::CallStatisticsRepository& call_statistics)
    : hubs_{hubs}
    , servers_{servers}
    , nas_ips_{nas_ips}
    , trunks_{trunks}
    , pricelists_{pricelists}
    , tarifs_{tarifs}
    , call_statistics_{call_statistics} {}

std::vector<ui::detail::HubInfo> UseCasesImpl::GetHubs() const {
    return hubs_.Get();
}

std::vector<ui::detail::ServerInfo> UseCasesImpl::GetServers() const {
    return servers_.Get();
}

std::vector<ui::detail::NasIpInfo> UseCasesImpl::GetNasIps() const {
    return nas_ips_.Get();
}

std::vector<ui::detail::TrunkInfo> UseCasesImpl::GetTrunks() const {
    return trunks_.Get();
}

std::vector<ui::detail::PricelistInfo> UseCasesImpl::GetPricelists() const {
    return pricelists_.Get();
}

std::vector<ui::detail::TarifInfo> UseCasesImpl::GetTarifs() const {
    return tarifs_.Get();
}

std::vector<ui::detail::CallStatisticsInfo> UseCasesImpl::GetCallStatistics() const {
    return call_statistics_.Get();
}

void UseCasesImpl::AddPricelist(const ui::detail::PricelistInfo& pricelist) {
    auto worker = pricelists_.GetWorker();
    worker->AddPricelist({pricelist.id, pricelist.name, pricelist.currency,
                          pricelist.rate_per_minute, pricelist.is_active});
}

void UseCasesImpl::UpdatePricelist(const ui::detail::PricelistInfo& pricelist, int id) {
    auto worker = pricelists_.GetWorker();
    worker->UpdatePricelist({pricelist.id, pricelist.name, pricelist.currency,
                             pricelist.rate_per_minute, pricelist.is_active}, id);
}

void UseCasesImpl::AddTarif(const ui::detail::TarifInfo& tarif) {
    auto worker = tarifs_.GetWorker();
    worker->AddTarif({tarif.id, tarif.name, tarif.pricelist_id,
                      tarif.markup_percent, tarif.free_minutes});
}

void UseCasesImpl::UpdateTarif(const ui::detail::TarifInfo& tarif, int id) {
    auto worker = tarifs_.GetWorker();
    worker->UpdateTarif({tarif.id, tarif.name, tarif.pricelist_id,
                         tarif.markup_percent, tarif.free_minutes}, id);
}

void UseCasesImpl::AddTrunk(const ui::detail::TrunkInfo& trunk) {
    auto worker = trunks_.GetWorker();
    worker->AddTrunk({trunk.id, trunk.server_id, trunk.name,
                      trunk.capacity, trunk.cost_per_channel});
}

void UseCasesImpl::UpdateTrunk(const ui::detail::TrunkInfo& trunk, int id) {
    auto worker = trunks_.GetWorker();
    worker->UpdateTrunk({trunk.id, trunk.server_id, trunk.name,
                         trunk.capacity, trunk.cost_per_channel}, id);
}

void UseCasesImpl::AddCallStatistics(const ui::detail::CallStatisticsInfo& call_stat) {
    auto worker = call_statistics_.GetWorker();
    worker->AddCallStatistics({call_stat.id, call_stat.call_id, call_stat.trunk_id,
                               call_stat.tarif_id, call_stat.duration_seconds,
                               call_stat.cost, call_stat.call_time});
}

} // namespace app
