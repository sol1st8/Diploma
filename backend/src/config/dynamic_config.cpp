#include "dynamic_config.h"
#include <boost/json.hpp>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <map>
#include <iostream>

namespace config {

namespace json = boost::json;
using namespace std::literals;

// Глобальный экземпляр
DynamicConfig g_config;

// Реализация вспомогательных методов AppConfiguration
std::string AppConfiguration::GetCentralDbUrl() const {
    return "postgresql://" + central_user + ":" + central_password + "@" + 
           central_host + ":" + std::to_string(central_port) + "/" + central_database;
}

std::string AppConfiguration::GetRegionalDbUrl() const {
    return "postgresql://" + regional_user + ":" + regional_password + "@" + 
           regional_host + ":" + std::to_string(regional_port) + "/" + regional_database;
}

DynamicConfig::DynamicConfig() {
    // Инициализируем с конфигурацией по умолчанию
    config_.store(std::make_shared<AppConfiguration>());
}

DynamicConfig::~DynamicConfig() {
    StopFileWatcher();
}

std::shared_ptr<const AppConfiguration> DynamicConfig::Get() const {
    // Потокобезопасное чтение
    return config_.load();
}

void DynamicConfig::Update(std::shared_ptr<AppConfiguration> new_config) {
    // Увеличиваем версию
    auto current = config_.load();
    new_config->version = current->version + 1;
    new_config->last_updated = GetCurrentTimestamp();
    
    // Потокобезопасная запись
    config_.store(new_config);
}

std::string DynamicConfig::GetCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// Парсинг application.conf
std::shared_ptr<AppConfiguration> DynamicConfig::ParseConfigFile(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file: "s + file_path);
    }
    
    auto cfg = std::make_shared<AppConfiguration>();
    std::string line;
    std::string current_section;
    
    while (std::getline(file, line)) {
        // Убираем пробелы
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        // Пропускаем пустые строки и комментарии
        if (line.empty() || line[0] == '#') continue;
        
        // Определяем секцию
        if (line.find('{') != std::string::npos) {
            size_t pos = line.find('{');
            current_section = line.substr(0, pos);
            current_section.erase(current_section.find_last_not_of(" \t") + 1);
            continue;
        }
        
        if (line.find('}') != std::string::npos) {
            current_section.clear();
            continue;
        }
        
        // Парсим параметры
        size_t eq_pos = line.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = line.substr(0, eq_pos);
            std::string value = line.substr(eq_pos + 1);
            
            // Убираем пробелы
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // Убираем кавычки из значения
            if (!value.empty() && value.front() == '"') value.erase(0, 1);
            if (!value.empty() && value.back() == '"') value.pop_back();
            
            // Парсим значения в зависимости от секции
            if (current_section == "sync") {
                if (key == "enabled") {
                    cfg->sync_enabled = (value == "true");
                } else if (key == "interval_seconds") {
                    cfg->sync_interval_seconds = std::stoi(value);
                }
            }
            else if (current_section == "sync.central_db") {
                if (key == "host") cfg->central_host = value;
                else if (key == "port") cfg->central_port = std::stoi(value);
                else if (key == "database") cfg->central_database = value;
                else if (key == "user") cfg->central_user = value;
                else if (key == "password") cfg->central_password = value;
                else if (key == "server_id") cfg->central_server_id = std::stoi(value);
            }
            else if (current_section == "sync.regional_db") {
                if (key == "host") cfg->regional_host = value;
                else if (key == "port") cfg->regional_port = std::stoi(value);
                else if (key == "database") cfg->regional_database = value;
                else if (key == "user") cfg->regional_user = value;
                else if (key == "password") cfg->regional_password = value;
                else if (key == "server_id") cfg->regional_server_id = std::stoi(value);
            }
            else if (current_section == "call_simulator") {
                if (key == "default_call_count") cfg->default_call_count = std::stoi(value);
                else if (key == "min_call_duration") cfg->min_call_duration = std::stoi(value);
                else if (key == "max_call_duration") cfg->max_call_duration = std::stoi(value);
                else if (key == "max_calls_per_request") cfg->max_calls_per_request = std::stoi(value);
            }
        }
    }
    
    cfg->last_updated = GetCurrentTimestamp();
    return cfg;
}

// Генерация application.conf
std::string DynamicConfig::GenerateConfigFile(const AppConfiguration& cfg) const {
    std::stringstream ss;
    
    ss << "sync {\n";
    ss << "    enabled = " << (cfg.sync_enabled ? "true" : "false") << "\n";
    ss << "    \n";
    ss << "    interval_seconds = " << cfg.sync_interval_seconds << "\n";
    ss << "    \n";
    ss << "    central_db {\n";
    ss << "        host = \"" << cfg.central_host << "\"\n";
    ss << "        port = " << cfg.central_port << "\n";
    ss << "        database = \"" << cfg.central_database << "\"\n";
    ss << "        user = \"" << cfg.central_user << "\"\n";
    ss << "        password = \"" << cfg.central_password << "\"\n";
    ss << "        server_id = " << cfg.central_server_id << "\n";
    ss << "    }\n";
    ss << "    \n";
    ss << "    regional_db {\n";
    ss << "        host = \"" << cfg.regional_host << "\"\n";
    ss << "        port = " << cfg.regional_port << "\n";
    ss << "        database = \"" << cfg.regional_database << "\"\n";
    ss << "        user = \"" << cfg.regional_user << "\"\n";
    ss << "        password = \"" << cfg.regional_password << "\"\n";
    ss << "        server_id = " << cfg.regional_server_id << "\n";
    ss << "    }\n";
    ss << "}\n";
    ss << "\n";
    ss << "# Параметры симуляции звонков\n";
    ss << "call_simulator {\n";
    ss << "    default_call_count = " << cfg.default_call_count << "\n";
    ss << "    min_call_duration = " << cfg.min_call_duration << "\n";
    ss << "    max_call_duration = " << cfg.max_call_duration << "\n";
    ss << "    max_calls_per_request = " << cfg.max_calls_per_request << "\n";
    ss << "}\n";
    
    return ss.str();
}

void DynamicConfig::LoadFromFile(const std::string& file_path) {
    auto new_config = ParseConfigFile(file_path);
    config_file_path_ = file_path;
    Update(new_config);
}

void DynamicConfig::SaveToFile(const std::string& file_path) const {
    std::ofstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file for writing: "s + file_path);
    }
    
    auto cfg = Get();
    file << GenerateConfigFile(*cfg);
}

std::string DynamicConfig::ToJson() const {
    auto cfg = Get();
    
    json::object obj{
        {"sync_enabled"s, cfg->sync_enabled},
        {"sync_interval_seconds"s, cfg->sync_interval_seconds},
        {"central_db"s, {
            {"host"s, cfg->central_host},
            {"port"s, cfg->central_port},
            {"database"s, cfg->central_database},
            {"user"s, cfg->central_user},
            {"password"s, cfg->central_password},
            {"server_id"s, cfg->central_server_id}
        }},
        {"regional_db"s, {
            {"host"s, cfg->regional_host},
            {"port"s, cfg->regional_port},
            {"database"s, cfg->regional_database},
            {"user"s, cfg->regional_user},
            {"password"s, cfg->regional_password},
            {"server_id"s, cfg->regional_server_id}
        }},
        {"default_call_count"s, cfg->default_call_count},
        {"min_call_duration"s, cfg->min_call_duration},
        {"max_call_duration"s, cfg->max_call_duration},
        {"max_calls_per_request"s, cfg->max_calls_per_request},
        {"version"s, cfg->version},
        {"last_updated"s, cfg->last_updated}
    };
    
    return json::serialize(obj);
}

void DynamicConfig::UpdateFromJson(const std::string& json_str) {
    try {
        json::value jv = json::parse(json_str);
        auto obj = jv.as_object();
        
        auto new_config = std::make_shared<AppConfiguration>(*Get());
        
        // Обновляем sync параметры
        if (obj.contains("sync_enabled"s)) {
            new_config->sync_enabled = obj.at("sync_enabled"s).as_bool();
        }
        if (obj.contains("sync_interval_seconds"s)) {
            new_config->sync_interval_seconds = obj.at("sync_interval_seconds"s).as_int64();
        }
        
        // Обновляем central_db
        if (obj.contains("central_db"s)) {
            auto central = obj.at("central_db"s).as_object();
            if (central.contains("host"s)) new_config->central_host = central.at("host"s).as_string().c_str();
            if (central.contains("port"s)) new_config->central_port = central.at("port"s).as_int64();
            if (central.contains("database"s)) new_config->central_database = central.at("database"s).as_string().c_str();
            if (central.contains("user"s)) new_config->central_user = central.at("user"s).as_string().c_str();
            if (central.contains("password"s)) new_config->central_password = central.at("password"s).as_string().c_str();
            if (central.contains("server_id"s)) new_config->central_server_id = central.at("server_id"s).as_int64();
        }
        
        // Обновляем regional_db
        if (obj.contains("regional_db"s)) {
            auto regional = obj.at("regional_db"s).as_object();
            if (regional.contains("host"s)) new_config->regional_host = regional.at("host"s).as_string().c_str();
            if (regional.contains("port"s)) new_config->regional_port = regional.at("port"s).as_int64();
            if (regional.contains("database"s)) new_config->regional_database = regional.at("database"s).as_string().c_str();
            if (regional.contains("user"s)) new_config->regional_user = regional.at("user"s).as_string().c_str();
            if (regional.contains("password"s)) new_config->regional_password = regional.at("password"s).as_string().c_str();
            if (regional.contains("server_id"s)) new_config->regional_server_id = regional.at("server_id"s).as_int64();
        }
        
        // Обновляем дополнительные параметры
        if (obj.contains("default_call_count"s)) {
            new_config->default_call_count = obj.at("default_call_count"s).as_int64();
        }
        if (obj.contains("min_call_duration"s)) {
            new_config->min_call_duration = obj.at("min_call_duration"s).as_int64();
        }
        if (obj.contains("max_call_duration"s)) {
            new_config->max_call_duration = obj.at("max_call_duration"s).as_int64();
        }
        if (obj.contains("max_calls_per_request"s)) {
            new_config->max_calls_per_request = obj.at("max_calls_per_request"s).as_int64();
        }
        
        Update(new_config);
        
        // Сохраняем в файл если путь известен
        if (!config_file_path_.empty()) {
            SaveToFile(config_file_path_);
        }
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse JSON configuration: "s + e.what());
    }
}

void DynamicConfig::StartFileWatcher(const std::string& file_path, int interval_seconds) {
    StopFileWatcher();
    
    config_file_path_ = file_path;
    watcher_running_ = true;
    
    watcher_thread_ = std::make_unique<std::thread>([this, file_path, interval_seconds]() {
        while (watcher_running_) {
            try {
                // Перечитываем файл
                auto new_config = ParseConfigFile(file_path);
                
                // Проверяем, изменилась ли конфигурация
                auto current = Get();
                bool changed = false;
                
                // Простая проверка - сравниваем ключевые параметры
                if (new_config->sync_enabled != current->sync_enabled ||
                    new_config->sync_interval_seconds != current->sync_interval_seconds ||
                    new_config->central_host != current->central_host ||
                    new_config->regional_host != current->regional_host) {
                    changed = true;
                }
                
                if (changed) {
                    Update(new_config);
                    std::cout << "[DynamicConfig] Configuration reloaded from file (version: " 
                             << new_config->version << ")" << std::endl;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "[DynamicConfig] Error reloading config: " << e.what() << std::endl;
            }
            
            // Ждем перед следующей проверкой
            for (int i = 0; i < interval_seconds && watcher_running_; ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    });
}

void DynamicConfig::StopFileWatcher() {
    if (watcher_thread_ && watcher_running_) {
        watcher_running_ = false;
        if (watcher_thread_->joinable()) {
            watcher_thread_->join();
        }
        watcher_thread_.reset();
    }
}

int DynamicConfig::GetVersion() const {
    return Get()->version;
}

std::string DynamicConfig::GetConfigFilePath() const {
    return config_file_path_;
}

sync_load::SyncConfig DynamicConfig::GetSyncConfig() const {
    auto cfg = Get();
    
    sync_load::SyncConfig sync_cfg;
    sync_cfg.central_db_url = cfg->GetCentralDbUrl();
    sync_cfg.regional_db_url = cfg->GetRegionalDbUrl();
    sync_cfg.central_server_id = cfg->central_server_id;
    sync_cfg.regional_server_id = cfg->regional_server_id;
    sync_cfg.sync_interval_seconds = cfg->sync_enabled ? cfg->sync_interval_seconds : 0;
    
    return sync_cfg;
}

} // namespace config
