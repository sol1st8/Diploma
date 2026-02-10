#pragma once

#include "../connection_pool.h"
#include "sync_config.h"

#include <pqxx/connection>
#include <pqxx/transaction>

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

namespace sync_load {

using EventHandler = std::function<void(pqxx::connection&, const std::string&)>;

class EventLoader {
public:
    explicit EventLoader(const SyncConfig& config);
    
    void Initialize();
    void ProcessEvents();
    
    std::string GetCurrentEvent() const;
    int GetEventCount() const;
    
private:
    void InitHandlers();
    void ProcessEventsFromCentralToRegional();
    void ProcessEventsFromRegionalToCentral();
    void HandleEvent(const std::string& event_name, long long version,
                    pqxx::connection& target_conn, const std::string& source_conn_str,
                    bool is_central_to_regional);
    
    bool HasEvent(const std::string& event) const;
    void AddEvent(const std::string& event);
    void RemoveEvent(const std::string& event);
    
    SyncConfig config_;
    std::unique_ptr<pqxx::connection> central_conn_;  // Подключение к центральной базе
    std::unique_ptr<pqxx::connection> regional_conn_; // Подключение к региональной базе
    
    std::map<std::string, EventHandler> handlers_;
    
    mutable std::mutex mutex_;
    std::set<std::string> active_events_;
    
    std::string current_event_;
    int event_count_ = 0;
};

} // namespace sync_load
