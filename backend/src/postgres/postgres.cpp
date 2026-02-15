#include "postgres.h"
#include "../ui/view.h"

#include <pqxx/zview.hxx>
#include <pqxx/pqxx>

#include <stdexcept>

#include <iostream>

namespace postgres {
using namespace std::literals;
using pqxx::operator"" _zv;

std::vector<ui::detail::HubInfo> HubRepositoryImpl::Get() const {
    auto conn = pool_.GetConnection();
    pqxx::read_transaction tr(*conn);

    std::string query = "SELECT * FROM hub ORDER BY id;"s;

    auto resp = tr.query<int, std::string, std::string, bool>(query);

    std::vector<ui::detail::HubInfo> result;

    for (const auto& [id, name, location, is_active] : resp) {
        ui::detail::HubInfo hub{id, name, location, is_active};
        result.push_back(hub);
    }

    return result;
}

std::vector<ui::detail::ServerInfo> ServerRepositoryImpl::Get() const {
    auto conn = pool_.GetConnection();
    pqxx::read_transaction tr(*conn);

    std::string query = "SELECT * FROM server ORDER BY id;"s;

    auto resp = tr.query<int, int, std::string, bool>(query);

    std::vector<ui::detail::ServerInfo> result;

    for (const auto& [id, hub_id, name, is_active] : resp) {
        ui::detail::ServerInfo server{id, hub_id, name, is_active};
        result.push_back(server);
    }

    return result;
}

std::vector<ui::detail::NasIpInfo> NasIpRepositoryImpl::Get() const {
    auto conn = pool_.GetConnection();
    pqxx::read_transaction tr(*conn);

    std::string query = "SELECT * FROM nas_ip ORDER BY id;"s;

    auto resp = tr.query<int, int, std::string, std::string>(query);

    std::vector<ui::detail::NasIpInfo> result;

    for (const auto& [id, server_id, ip_address, description] : resp) {
        ui::detail::NasIpInfo nas_ip{id, server_id, ip_address, description};
        result.push_back(nas_ip);
    }

    return result;
}

std::vector<ui::detail::TrunkInfo> TrunkRepositoryImpl::Get() const {
    auto conn = pool_.GetConnection();
    pqxx::read_transaction tr(*conn);

    std::string query = "SELECT * FROM trunk ORDER BY id;"s;

    auto resp = tr.query<int, int, std::string, int, double>(query);

    std::vector<ui::detail::TrunkInfo> result;

    for (const auto& [id, server_id, name, capacity, cost_per_channel] : resp) {
        ui::detail::TrunkInfo trunk{id, server_id, name, capacity, cost_per_channel};
        result.push_back(trunk);
    }

    return result;
}

std::vector<ui::detail::PricelistInfo> PricelistRepositoryImpl::Get() const {
    auto conn = pool_.GetConnection();
    pqxx::read_transaction tr(*conn);

    std::string query = "SELECT * FROM pricelist ORDER BY id;"s;

    auto resp = tr.query<int, std::string, std::string, double, bool>(query);

    std::vector<ui::detail::PricelistInfo> result;

    for (const auto& [id, name, currency, rate_per_minute, is_active] : resp) {
        ui::detail::PricelistInfo pricelist{id, name, currency, rate_per_minute, is_active};
        result.push_back(pricelist);
    }

    return result;
}

std::vector<ui::detail::TarifInfo> TarifRepositoryImpl::Get() const {
    auto conn = pool_.GetConnection();
    pqxx::read_transaction tr(*conn);

    std::string query = "SELECT * FROM tarif ORDER BY id;"s;

    auto resp = tr.query<int, std::string, int, int, int>(query);

    std::vector<ui::detail::TarifInfo> result;

    for (const auto& [id, name, pricelist_id, markup_percent, free_minutes] : resp) {
        ui::detail::TarifInfo tarif{id, name, pricelist_id, markup_percent, free_minutes};
        result.push_back(tarif);
    }

    return result;
}

std::vector<ui::detail::CallStatisticsInfo> CallStatisticsRepositoryImpl::Get() const {
    auto conn = pool_.GetConnection();
    pqxx::read_transaction tr(*conn);

    std::string query = "SELECT * FROM call_statistics ORDER BY id;"s;

    auto resp = tr.query<int64_t, std::string, int, int, int, double, std::string>(query);

    std::vector<ui::detail::CallStatisticsInfo> result;

    for (const auto& [id, call_id, trunk_id, tarif_id, duration_seconds, cost, call_time] : resp) {
        ui::detail::CallStatisticsInfo call_stat{id, call_id, trunk_id, tarif_id, duration_seconds, cost, call_time};
        result.push_back(call_stat);
    }

    return result;
}

DataBase::DataBase(const std::string& db_url)
    : pool_{std::thread::hardware_concurrency(),
  [&db_url](){ return std::make_shared<pqxx::connection>(db_url); } }
    , hubs_{pool_}
    , servers_{pool_}
    , nas_ips_{pool_}
    , trunks_{pool_}
    , pricelists_{pool_}
    , tarifs_{pool_}
    , call_statistics_{pool_} {}

WorkerImpl::WorkerImpl(pqxx::connection& conn) : conn_(conn), nontr_(conn) {}

void WorkerImpl::AddPricelist(const domain::Pricelist& pricelist) {
    nontr_.exec_params(
        R"(
    INSERT INTO pricelist (id, name, currency, rate_per_minute, is_active) VALUES
                         ($1, $2, $3, $4, $5);
    )"_zv,
        pricelist.GetId(), pricelist.GetName(), pricelist.GetCurrency(),
        pricelist.GetRatePerMinute(), pricelist.GetIsActive());
}

void WorkerImpl::UpdatePricelist(const domain::Pricelist& pricelist, int id) {
    nontr_.exec_params(
        R"(
    UPDATE pricelist SET name=$2, currency=$3, rate_per_minute=$4, is_active=$5 WHERE id=$1;
    )"_zv,
        id, pricelist.GetName(), pricelist.GetCurrency(),
        pricelist.GetRatePerMinute(), pricelist.GetIsActive());
}

void WorkerImpl::AddTarif(const domain::Tarif& tarif) {
    nontr_.exec_params(
        R"(
    INSERT INTO tarif (id, name, pricelist_id, markup_percent, free_minutes) VALUES
                     ($1, $2, $3, $4, $5);
    )"_zv,
        tarif.GetId(), tarif.GetName(), tarif.GetPricelistId(),
        tarif.GetMarkupPercent(), tarif.GetFreeMinutes());
}

void WorkerImpl::UpdateTarif(const domain::Tarif& tarif, int id) {
    nontr_.exec_params(
        R"(
    UPDATE tarif SET name=$2, pricelist_id=$3, markup_percent=$4, free_minutes=$5 WHERE id=$1;
    )"_zv,
        id, tarif.GetName(), tarif.GetPricelistId(),
        tarif.GetMarkupPercent(), tarif.GetFreeMinutes());
}

void WorkerImpl::AddTrunk(const domain::Trunk& trunk) {
    nontr_.exec_params(
        R"(
    INSERT INTO trunk (id, server_id, name, capacity, cost_per_channel) VALUES
                     ($1, $2, $3, $4, $5);
    )"_zv,
        trunk.GetId(), trunk.GetServerId(), trunk.GetName(),
        trunk.GetCapacity(), trunk.GetCostPerChannel());
}

void WorkerImpl::UpdateTrunk(const domain::Trunk& trunk, int id) {
    nontr_.exec_params(
        R"(
    UPDATE trunk SET server_id=$2, name=$3, capacity=$4, cost_per_channel=$5 WHERE id=$1;
    )"_zv,
        id, trunk.GetServerId(), trunk.GetName(),
        trunk.GetCapacity(), trunk.GetCostPerChannel());
}

void WorkerImpl::AddCallStatistics(const domain::CallStatistics& call_stat) {
    nontr_.exec_params(
        R"(
    INSERT INTO call_statistics (call_id, trunk_id, tarif_id, duration_seconds, cost, call_time)
    VALUES ($1, $2, $3, $4, $5, $6);
    )"_zv,
        call_stat.GetCallId(), call_stat.GetTrunkId(), call_stat.GetTarifId(),
        call_stat.GetDurationSeconds(), call_stat.GetCost(), call_stat.GetCallTime());
}

WorkerImpl::~WorkerImpl() = default;

} // namespace postgres
