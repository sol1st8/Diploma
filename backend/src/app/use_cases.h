#pragma once

#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace ui {

namespace detail {

struct HubInfo;
struct ServerInfo;
struct NasIpInfo;
struct TrunkInfo;
struct PricelistInfo;
struct TarifInfo;
struct CallStatisticsInfo;

} // namespace detail

} // namespace ui

namespace app {

class UseCases {
  public:
    virtual std::vector<ui::detail::HubInfo> GetHubs() const = 0;
    virtual std::vector<ui::detail::ServerInfo> GetServers() const = 0;
    virtual std::vector<ui::detail::NasIpInfo> GetNasIps() const = 0;
    virtual std::vector<ui::detail::TrunkInfo> GetTrunks() const = 0;
    virtual std::vector<ui::detail::PricelistInfo> GetPricelists() const = 0;
    virtual std::vector<ui::detail::TarifInfo> GetTarifs() const = 0;
    virtual std::vector<ui::detail::CallStatisticsInfo> GetCallStatistics() const = 0;

    virtual void AddPricelist(const ui::detail::PricelistInfo& pricelist) = 0;
    virtual void UpdatePricelist(const ui::detail::PricelistInfo& pricelist, int id) = 0;

    virtual void AddTarif(const ui::detail::TarifInfo& tarif) = 0;
    virtual void UpdateTarif(const ui::detail::TarifInfo& tarif, int id) = 0;

    virtual void AddTrunk(const ui::detail::TrunkInfo& trunk) = 0;
    virtual void UpdateTrunk(const ui::detail::TrunkInfo& trunk, int id) = 0;

    virtual void AddCallStatistics(const ui::detail::CallStatisticsInfo& call_stat) = 0;

  protected:
    ~UseCases() = default;
};

} // namespace app
