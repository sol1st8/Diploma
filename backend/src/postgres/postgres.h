#pragma once

#include "../connection_pool.h"
#include "../domain/worker.h"
#include "../domain/hub.h"
#include "../domain/server.h"
#include "../domain/nas_ip.h"
#include "../domain/trunk.h"
#include "../domain/pricelist.h"
#include "../domain/tarif.h"
#include "../domain/call_statistics.h"
#include "../ui/view.h"

#include <pqxx/connection>
#include <pqxx/transaction>

#include <memory>
#include <vector>

namespace postgres {

class WorkerImpl : public domain::Worker {
  public:
    explicit WorkerImpl(pqxx::connection& conn);

    void AddPricelist(const domain::Pricelist& pricelist) override;
    void UpdatePricelist(const domain::Pricelist& pricelist, int id) override;

    void AddTarif(const domain::Tarif& tarif) override;
    void UpdateTarif(const domain::Tarif& tarif, int id) override;

    void AddTrunk(const domain::Trunk& trunk) override;
    void UpdateTrunk(const domain::Trunk& trunk, int id) override;

    void AddCallStatistics(const domain::CallStatistics& call_stat) override;

    ~WorkerImpl() override;

  private:
    pqxx::connection& conn_;
    pqxx::nontransaction nontr_;
};

class HubRepositoryImpl : public domain::HubRepository {
  public:
    explicit HubRepositoryImpl(connection_pool::ConnectionPool& pool) : pool_{pool} {}

    std::vector<ui::detail::HubInfo> Get() const override;

    std::shared_ptr<domain::Worker> GetWorker() const override {
        auto conn = pool_.GetConnection();
        return std::make_shared<WorkerImpl>(*conn);
    }

  private:
    connection_pool::ConnectionPool& pool_;
};

class ServerRepositoryImpl : public domain::ServerRepository {
  public:
    explicit ServerRepositoryImpl(connection_pool::ConnectionPool& pool) : pool_{pool} {}

    std::vector<ui::detail::ServerInfo> Get() const override;

    std::shared_ptr<domain::Worker> GetWorker() const override {
        auto conn = pool_.GetConnection();
        return std::make_shared<WorkerImpl>(*conn);
    }

  private:
    connection_pool::ConnectionPool& pool_;
};

class NasIpRepositoryImpl : public domain::NasIpRepository {
  public:
    explicit NasIpRepositoryImpl(connection_pool::ConnectionPool& pool) : pool_{pool} {}

    std::vector<ui::detail::NasIpInfo> Get() const override;

    std::shared_ptr<domain::Worker> GetWorker() const override {
        auto conn = pool_.GetConnection();
        return std::make_shared<WorkerImpl>(*conn);
    }

  private:
    connection_pool::ConnectionPool& pool_;
};

class TrunkRepositoryImpl : public domain::TrunkRepository {
  public:
    explicit TrunkRepositoryImpl(connection_pool::ConnectionPool& pool) : pool_{pool} {}

    std::vector<ui::detail::TrunkInfo> Get() const override;

    std::shared_ptr<domain::Worker> GetWorker() const override {
        auto conn = pool_.GetConnection();
        return std::make_shared<WorkerImpl>(*conn);
    }

  private:
    connection_pool::ConnectionPool& pool_;
};

class PricelistRepositoryImpl : public domain::PricelistRepository {
  public:
    explicit PricelistRepositoryImpl(connection_pool::ConnectionPool& pool) : pool_{pool} {}

    std::vector<ui::detail::PricelistInfo> Get() const override;

    std::shared_ptr<domain::Worker> GetWorker() const override {
        auto conn = pool_.GetConnection();
        return std::make_shared<WorkerImpl>(*conn);
    }

  private:
    connection_pool::ConnectionPool& pool_;
};

class TarifRepositoryImpl : public domain::TarifRepository {
  public:
    explicit TarifRepositoryImpl(connection_pool::ConnectionPool& pool) : pool_{pool} {}

    std::vector<ui::detail::TarifInfo> Get() const override;

    std::shared_ptr<domain::Worker> GetWorker() const override {
        auto conn = pool_.GetConnection();
        return std::make_shared<WorkerImpl>(*conn);
    }

  private:
    connection_pool::ConnectionPool& pool_;
};

class CallStatisticsRepositoryImpl : public domain::CallStatisticsRepository {
  public:
    explicit CallStatisticsRepositoryImpl(connection_pool::ConnectionPool& pool) : pool_{pool} {}

    std::vector<ui::detail::CallStatisticsInfo> Get() const override;

    std::shared_ptr<domain::Worker> GetWorker() const override {
        auto conn = pool_.GetConnection();
        return std::make_shared<WorkerImpl>(*conn);
    }

  private:
    connection_pool::ConnectionPool& pool_;
};

class DataBase {
  public:
    explicit DataBase(const std::string& db_url);

    HubRepositoryImpl& GetHubs() & {
        return hubs_;
    }

    ServerRepositoryImpl& GetServers() & {
        return servers_;
    }

    NasIpRepositoryImpl& GetNasIps() & {
        return nas_ips_;
    }

    TrunkRepositoryImpl& GetTrunks() & {
        return trunks_;
    }

    PricelistRepositoryImpl& GetPricelists() & {
        return pricelists_;
    }

    TarifRepositoryImpl& GetTarifs() & {
        return tarifs_;
    }

    CallStatisticsRepositoryImpl& GetCallStatistics() & {
        return call_statistics_;
    }

  private:
    connection_pool::ConnectionPool pool_;

    HubRepositoryImpl hubs_;
    ServerRepositoryImpl servers_;
    NasIpRepositoryImpl nas_ips_;
    TrunkRepositoryImpl trunks_;
    PricelistRepositoryImpl pricelists_;
    TarifRepositoryImpl tarifs_;
    CallStatisticsRepositoryImpl call_statistics_;
};

}  // namespace postgres
