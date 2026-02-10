#pragma once

#include "app/use_cases_impl.h"
#include "postgres/postgres.h"

#include <pqxx/pqxx>

namespace db {

struct AppConfig {
    std::string db_url;
};

class Application {
  public:
    explicit Application(const AppConfig& config);

    app::UseCasesImpl GetUseCases() const;

  private:
    postgres::DataBase db_;
    app::UseCasesImpl use_cases_{db_.GetHubs(), db_.GetServers(),
                                 db_.GetNasIps(), db_.GetTrunks(),
                                 db_.GetPricelists(), db_.GetTarifs(),
                                 db_.GetCallStatistics()};
};

} // namespace db
