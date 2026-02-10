#pragma once

#include <string>

namespace sync_load {

struct SyncConfig {
    std::string central_db_url;
    std::string regional_db_url;
    int central_server_id = 777;
    int regional_server_id = 159;
    int sync_interval_seconds = 10;
};

} // namespace sync_load
