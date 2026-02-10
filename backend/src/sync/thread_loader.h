#pragma once

#include "event_loader.h"
#include "sync_config.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

namespace sync_load {

class ThreadLoader {
public:
    explicit ThreadLoader(const SyncConfig& config);
    ~ThreadLoader();
    
    void Start();
    void Stop();
    
    bool IsRunning() const;
    //std::string GetStatus() const;
    
private:
    void Run();
    
    SyncConfig config_;
    std::unique_ptr<EventLoader> loader_;
    
    std::unique_ptr<std::thread> thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
};

} // namespace sync_load
