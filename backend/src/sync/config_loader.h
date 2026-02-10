#pragma once

#include "sync_config.h"

#include <string>
#include <fstream>
#include <sstream>
#include <map>

namespace sync_load {

class ConfigLoader {
public:
    static SyncConfig LoadFromFile(const std::string& config_path);
    
private:
    static std::map<std::string, std::string> ParseConfig(const std::string& content);
    static std::string GetValue(const std::map<std::string, std::string>& config, 
                                const std::string& key, 
                                const std::string& default_value = "");
    static int GetIntValue(const std::map<std::string, std::string>& config, 
                          const std::string& key, 
                          int default_value = 0);
    static bool GetBoolValue(const std::map<std::string, std::string>& config, 
                            const std::string& key, 
                            bool default_value = false);
    
    static std::string BuildConnectionString(const std::string& host, 
                                            int port, 
                                            const std::string& database,
                                            const std::string& user, 
                                            const std::string& password);
};

} // namespace sync_load
