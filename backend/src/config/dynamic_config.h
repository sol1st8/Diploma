#pragma once

#include "../sync/sync_config.h"
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <chrono>

namespace config {

// Расширенная конфигурация приложения на основе application.conf
// Значения по умолчанию ПОЛНОСТЬЮ совпадают с backend/application.conf
struct AppConfiguration {
    // Из application.conf (sync секция)
    bool sync_enabled = true;              // sync.enabled
    int sync_interval_seconds = 60;        // sync.interval_seconds
    
    // Central DB (sync.central_db) - значения из application.conf
    std::string central_host = "172.17.0.3";
    int central_port = 5432;
    std::string central_database = "central";
    std::string central_user = "postgres";
    std::string central_password = "4317321";
    int central_server_id = 777;
    
    // Regional DB (sync.regional_db) - значения из application.conf
    std::string regional_host = "172.17.0.3";
    int regional_port = 5432;
    std::string regional_database = "regional";
    std::string regional_user = "postgres";
    std::string regional_password = "4317321";
    int regional_server_id = 159;
    
    int default_call_count = 10;
    int min_call_duration = 30;
    int max_call_duration = 600;
    int max_calls_per_request = 1000;
    
    // Версия конфигурации (автоматически увеличивается)
    int version = 1;
    
    // Время последнего обновления (автоматически)
    std::string last_updated;
    
    // Вспомогательные методы для получения connection strings
    std::string GetCentralDbUrl() const;
    std::string GetRegionalDbUrl() const;
};

// Класс для управления динамической конфигурацией
class DynamicConfig {
public:
    DynamicConfig();
    ~DynamicConfig();
    
    // Получить текущую конфигурацию (потокобезопасно через atomic)
    std::shared_ptr<const AppConfiguration> Get() const;
    
    // Обновить конфигурацию (потокобезопасно через atomic)
    void Update(std::shared_ptr<AppConfiguration> new_config);
    
    // Загрузить конфигурацию из application.conf
    void LoadFromFile(const std::string& file_path);
    
    // Сохранить конфигурацию в application.conf
    void SaveToFile(const std::string& file_path) const;
    
    // Получить конфигурацию в виде JSON
    std::string ToJson() const;
    
    // Обновить конфигурацию из JSON
    void UpdateFromJson(const std::string& json_str);
    
    // Запустить автоматическое перечитывание файла
    void StartFileWatcher(const std::string& file_path, int interval_seconds = 10);
    
    // Остановить автоматическое перечитывание
    void StopFileWatcher();
    
    // Получить версию конфигурации
    int GetVersion() const;
    
    // Получить путь к файлу конфигурации
    std::string GetConfigFilePath() const;
    
    // Получить SyncConfig для совместимости со старым кодом
    sync_load::SyncConfig GetSyncConfig() const;

private:
    // Потокобезопасный доступ к конфигурации
    std::atomic<std::shared_ptr<AppConfiguration>> config_;
    
    // Для file watcher
    std::unique_ptr<std::thread> watcher_thread_;
    std::atomic<bool> watcher_running_{false};
    std::string config_file_path_;
    
    // Парсинг application.conf
    std::shared_ptr<AppConfiguration> ParseConfigFile(const std::string& file_path);
    
    // Генерация application.conf из конфигурации
    std::string GenerateConfigFile(const AppConfiguration& cfg) const;
    
    // Получить текущее время в строковом формате
    std::string GetCurrentTimestamp() const;
};

// Глобальный экземпляр конфигурации
extern DynamicConfig g_config;

} // namespace config
