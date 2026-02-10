#include "thread_loader.h"
#include "../logger/logger.h"

namespace sync_load {

ThreadLoader::ThreadLoader(const SyncConfig& config) 
    : config_(config)
    , loader_(std::make_unique<EventLoader>(config)) {
}

ThreadLoader::~ThreadLoader() {
    Stop();
}

void ThreadLoader::Start() {
    if (running_) {
        LOG_INFO("ThreadLoader is already running");
        return;
    }
    
    try {
        loader_->Initialize();
        
        stop_requested_ = false;
        running_ = true;
        
        thread_ = std::make_unique<std::thread>(&ThreadLoader::Run, this);
        
        LOG_INFO("ThreadLoader started successfully");
    } 
    catch (const std::exception& e) {
        running_ = false;
        LOG_ERROR("Failed to start ThreadLoader: " + std::string(e.what()));
        throw;
    }
}

void ThreadLoader::Stop() {
    if (!running_) {
        return;
    }
    
    LOG_INFO("Stopping ThreadLoader...");
    
    stop_requested_ = true;
    
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
    
    running_ = false;
    LOG_INFO("ThreadLoader stopped");
}

void ThreadLoader::Run() {
    LOG_INFO("ThreadLoader main loop started");
    
    while (!stop_requested_) {
        try {
            loader_->ProcessEvents();
            
            std::this_thread::sleep_for(
                std::chrono::seconds(config_.sync_interval_seconds)
            );
            
        } 
        catch (const std::exception& e) {
            LOG_ERROR("Error in ThreadLoader loop: " + std::string(e.what()));
            
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    
    LOG_INFO("ThreadLoader main loop finished");
}

bool ThreadLoader::IsRunning() const {
    return running_;
}

//std::string ThreadLoader::GetStatus() const { // Использовать
//    if (!running_) {
//        return "Stopped";
//    }
//    
//    return "Running - Current event: " + loader_->GetCurrentEvent() + 
//           ", Events count: " + std::to_string(loader_->GetEventCount());
//}

} // namespace sync_load
