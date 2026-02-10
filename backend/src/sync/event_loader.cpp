#include "event_loader.h"
#include "../logger/logger.h"

#include <stdexcept>

namespace sync_load {

EventLoader::EventLoader(const SyncConfig& config) 
    : config_(config) {
}

void EventLoader::Initialize() {
    try {
        // Подключение к центральной базе
        central_conn_ = std::make_unique<pqxx::connection>(config_.central_db_url);
        LOG_INFO("Connected to central database");
        
        // Подключение к региональной базе
        regional_conn_ = std::make_unique<pqxx::connection>(config_.regional_db_url);
        LOG_INFO("Connected to regional database");
        
        InitHandlers();
        LOG_INFO("EventLoader initialized successfully");
    } 
    catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize EventLoader: " + std::string(e.what()));
        throw;
    }
}

void EventLoader::InitHandlers() {
    // Обработчики используют dblink для синхронизации
    handlers_ = {
        {"hub", [this](pqxx::connection& target_conn, const std::string& source_conn_str) {
            pqxx::work txn(target_conn);
            auto result = txn.exec_params(
                "SELECT sync_hub_from_remote($1)", source_conn_str);
            int count = result[0][0].as<int>();
            txn.commit();
            LOG_INFO("Synced: hub (" + std::to_string(count) + " records)");
        }},
        
        {"server", [this](pqxx::connection& target_conn, const std::string& source_conn_str) {
            pqxx::work txn(target_conn);
            auto result = txn.exec_params(
                "SELECT sync_server_from_remote($1)", source_conn_str);
            int count = result[0][0].as<int>();
            txn.commit();
            LOG_INFO("Synced: server (" + std::to_string(count) + " records)");
        }},
        
        {"nas_ip", [this](pqxx::connection& target_conn, const std::string& source_conn_str) {
            pqxx::work txn(target_conn);
            auto result = txn.exec_params(
                "SELECT sync_nas_ip_from_remote($1)", source_conn_str);
            int count = result[0][0].as<int>();
            txn.commit();
            LOG_INFO("Synced: nas_ip (" + std::to_string(count) + " records)");
        }},
        
        {"trunk", [this](pqxx::connection& target_conn, const std::string& source_conn_str) {
            pqxx::work txn(target_conn);
            auto result = txn.exec_params(
                "SELECT sync_trunk_from_remote($1)", source_conn_str);
            int count = result[0][0].as<int>();
            txn.commit();
            LOG_INFO("Synced: trunk (" + std::to_string(count) + " records)");
        }},
        
        {"pricelist", [this](pqxx::connection& target_conn, const std::string& source_conn_str) {
            pqxx::work txn(target_conn);
            auto result = txn.exec_params(
                "SELECT sync_pricelist_from_remote($1)", source_conn_str);
            int count = result[0][0].as<int>();
            txn.commit();
            LOG_INFO("Synced: pricelist (" + std::to_string(count) + " records)");
        }},
        
        {"tarif", [this](pqxx::connection& target_conn, const std::string& source_conn_str) {
            pqxx::work txn(target_conn);
            auto result = txn.exec_params(
                "SELECT sync_tarif_from_remote($1)", source_conn_str);
            int count = result[0][0].as<int>();
            txn.commit();
            LOG_INFO("Synced: tarif (" + std::to_string(count) + " records)");
        }},
        
        {"call_statistics", [this](pqxx::connection& target_conn, const std::string& source_conn_str) {
            pqxx::work txn(target_conn);
            auto result = txn.exec_params(
                "SELECT sync_call_statistics_from_remote($1)", source_conn_str);
            int count = result[0][0].as<int>();
            txn.commit();
            LOG_INFO("Synced: call_statistics (" + std::to_string(count) + " records)");
        }}
    };
}

void EventLoader::ProcessEvents() {
    if (!central_conn_ || !central_conn_->is_open()) {
        throw std::runtime_error("Central database connection is not open");
    }
    
    if (!regional_conn_ || !regional_conn_->is_open()) {
        throw std::runtime_error("Regional database connection is not open");
    }
    
    try {
        // 1. Обработка событий из центральной базы для региональной
        ProcessEventsFromCentralToRegional();
        
        // 2. Обработка событий из региональной базы для центральной
        ProcessEventsFromRegionalToCentral();
        
    } 
    catch (const std::exception& e) {
        LOG_ERROR("Error processing events: " + std::string(e.what()));
        throw;
    }
}

void EventLoader::ProcessEventsFromCentralToRegional() {
    // Читаем события из центральной базы для региональной (server_id=159)
    pqxx::work txn(*central_conn_);
    std::string query = "SELECT event, version FROM event.queue WHERE server_id = " + 
                       std::to_string(config_.regional_server_id) + " ORDER BY version;";
    pqxx::result result = txn.exec(query);
    txn.commit();
    
    int count = result.size();
    if (count > 0) {
        LOG_INFO("[Central→Regional] Found " + std::to_string(count) + " events");
    }
    
    for (const auto& row : result) {
        std::string event = row["event"].as<std::string>();
        long long version = row["version"].as<long long>();
        
        current_event_ = event;
        HandleEvent(event, version, *regional_conn_, config_.central_db_url, true);
    }
}

void EventLoader::ProcessEventsFromRegionalToCentral() {
    // Читаем события из региональной базы
    pqxx::work txn(*regional_conn_);
    std::string query = "SELECT event, version FROM event.queue ORDER BY version;";
    pqxx::result result = txn.exec(query);
    txn.commit();
    
    int count = result.size();
    if (count > 0) {
        LOG_INFO("[Regional→Central] Found " + std::to_string(count) + " events");
    }
    
    for (const auto& row : result) {
        std::string event = row["event"].as<std::string>();
        long long version = row["version"].as<long long>();
        
        current_event_ = event;
        HandleEvent(event, version, *central_conn_, config_.regional_db_url, false);
    }
}

void EventLoader::HandleEvent(const std::string& event_name, long long version,
                              pqxx::connection& target_conn, const std::string& source_conn_str,
                              bool is_central_to_regional) {
    if (HasEvent(event_name)) {
        return;
    }
    
    auto handler_it = handlers_.find(event_name);
    if (handler_it == handlers_.end()) {
        // Удаляем неизвестное событие
        pqxx::work txn(is_central_to_regional ? *central_conn_ : *regional_conn_);
        std::string delete_query;
        if (is_central_to_regional) {
            delete_query = "DELETE FROM event.queue WHERE server_id = " + 
                          std::to_string(config_.regional_server_id) +
                          " AND event='" + event_name + 
                          "' AND version=" + std::to_string(version) + ";";
        } 
        else {
            delete_query = "DELETE FROM event.queue WHERE event='" + event_name + 
                          "' AND version=" + std::to_string(version) + ";";
        }
        txn.exec(delete_query);
        txn.commit();
        
        LOG_ERROR("Event " + event_name + " not found!");
        return;
    }
    
    AddEvent(event_name);
    
    try {
        // Выполняем синхронизацию
        handler_it->second(target_conn, source_conn_str);
        
        // Удаляем обработанное событие
        pqxx::work txn(is_central_to_regional ? *central_conn_ : *regional_conn_);
        std::string delete_query;
        if (is_central_to_regional) {
            delete_query = "DELETE FROM event.queue WHERE server_id = " + 
                          std::to_string(config_.regional_server_id) +
                          " AND event='" + event_name + 
                          "' AND version=" + std::to_string(version) + ";";
        } 
        else {
            delete_query = "DELETE FROM event.queue WHERE event='" + event_name + 
                          "' AND version=" + std::to_string(version) + ";";
        }
        txn.exec(delete_query);
        txn.commit();
        
    } 
    catch (const std::exception& e) {
        LOG_ERROR("Error handling event " + event_name + ": " + std::string(e.what()));
    }
    
    RemoveEvent(event_name);
}

bool EventLoader::HasEvent(const std::string& event) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_events_.find(event) != active_events_.end();
}

void EventLoader::AddEvent(const std::string& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    active_events_.insert(event);
}

void EventLoader::RemoveEvent(const std::string& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    active_events_.erase(event);
}

std::string EventLoader::GetCurrentEvent() const {
    return current_event_;
}

int EventLoader::GetEventCount() const {
    return event_count_;
}

} // namespace sync_load
