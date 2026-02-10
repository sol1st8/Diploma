#include "config_loader.h"
#include "../logger/logger.h"

#include <stdexcept>
#include <algorithm>

namespace sync_load {

SyncConfig ConfigLoader::LoadFromFile(const std::string& config_path) {
    std::ifstream file(config_path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file: " + config_path);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    auto config_map = ParseConfig(content);
    
    SyncConfig config;
    
    bool sync_enabled = GetBoolValue(config_map, "sync.enabled", true);
    if (!sync_enabled) {
        LOG_INFO("Sync is disabled in config");
        config.sync_interval_seconds = 0;
        return config;
    }
    
    config.sync_interval_seconds = GetIntValue(config_map, "sync.interval_seconds", 60);
    
    // Центральная база
    std::string central_host = GetValue(config_map, "sync.central_db.host", "172.17.0.3");
    int central_port = GetIntValue(config_map, "sync.central_db.port", 5432);
    std::string central_db = GetValue(config_map, "sync.central_db.database", "central");
    std::string central_user = GetValue(config_map, "sync.central_db.user", "postgres");
    std::string central_pass = GetValue(config_map, "sync.central_db.password", "4317321");
    config.central_server_id = GetIntValue(config_map, "sync.central_db.server_id", 777);
    
    config.central_db_url = BuildConnectionString(central_host, central_port, 
                                                  central_db, central_user, central_pass);
    
    // Региональная база
    std::string regional_host = GetValue(config_map, "sync.regional_db.host", "172.17.0.3");
    int regional_port = GetIntValue(config_map, "sync.regional_db.port", 5432);
    std::string regional_db = GetValue(config_map, "sync.regional_db.database", "regional");
    std::string regional_user = GetValue(config_map, "sync.regional_db.user", "postgres");
    std::string regional_pass = GetValue(config_map, "sync.regional_db.password", "4317321");
    config.regional_server_id = GetIntValue(config_map, "sync.regional_db.server_id", 159);
    
    config.regional_db_url = BuildConnectionString(regional_host, regional_port, 
                                                   regional_db, regional_user, regional_pass);
    
    LOG_INFO("Config loaded successfully");
    LOG_INFO("  Sync interval: " + std::to_string(config.sync_interval_seconds) + " seconds");
    LOG_INFO("  Central server ID: " + std::to_string(config.central_server_id));
    LOG_INFO("  Regional server ID: " + std::to_string(config.regional_server_id));
    
    return config;
}

std::map<std::string, std::string> ConfigLoader::ParseConfig(const std::string& content) {
    std::map<std::string, std::string> result;
    std::istringstream stream(content);
    std::string line;
    std::string current_section;
    
    while (std::getline(stream, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        if (line.back() == '{') {
            std::string section = line.substr(0, line.find('{'));
            section.erase(section.find_last_not_of(" \t") + 1);
            
            if (!current_section.empty()) {
                current_section += ".";
            }
            current_section += section;
            continue;
        }
        
        if (line == "}") {
            size_t pos = current_section.find_last_of('.');
            if (pos != std::string::npos) {
                current_section = current_section.substr(0, pos);
            } else {
                current_section.clear();
            }
            continue;
        }
        
        size_t eq_pos = line.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = line.substr(0, eq_pos);
            std::string value = line.substr(eq_pos + 1);
            
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            if (!value.empty() && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.length() - 2);
            }
            
            std::string full_key = current_section.empty() ? key : current_section + "." + key;
            result[full_key] = value;
        }
    }
    
    return result;
}

std::string ConfigLoader::GetValue(const std::map<std::string, std::string>& config, 
                                   const std::string& key, 
                                   const std::string& default_value) {
    auto it = config.find(key);
    return (it != config.end()) ? it->second : default_value;
}

int ConfigLoader::GetIntValue(const std::map<std::string, std::string>& config, 
                              const std::string& key, 
                              int default_value) {
    auto it = config.find(key);
    if (it != config.end()) {
        try {
            return std::stoi(it->second);
        } 
        catch (...) {
            return default_value;
        }
    }
    return default_value;
}

bool ConfigLoader::GetBoolValue(const std::map<std::string, std::string>& config, 
                               const std::string& key, 
                               bool default_value) {
    auto it = config.find(key);
    if (it != config.end()) {
        std::string value = it->second;
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        return (value == "true" || value == "1" || value == "yes");
    }
    return default_value;
}

std::string ConfigLoader::BuildConnectionString(const std::string& host, 
                                               int port, 
                                               const std::string& database,
                                               const std::string& user, 
                                               const std::string& password) {
    return "postgresql://" + user + ":" + password + "@" + 
           host + ":" + std::to_string(port) + "/" + database;
}

} // namespace sync_load
