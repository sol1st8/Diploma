#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <deque>
#include <vector>

namespace logger {

enum class LogLevel {
    INFO,
    WARNING,
    ERROR,
    DEBUG
};

class Logger {
public:
    static Logger& Instance() {
        static Logger instance;
        return instance;
    }

    void Log(LogLevel level, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        ss << " [" << LevelToString(level) << "] " << message;
        
        std::string log_line = ss.str();
        
        if (log_file_.is_open()) {
            log_file_ << log_line << std::endl;
            log_file_.flush();
        }
        
        // Сохраняем в памяти (последние 1000 записей)
        log_buffer_.push_back(log_line);
        if (log_buffer_.size() > 1000) {
            log_buffer_.pop_front();
        }
    }

    void Info(const std::string& message) {
        Log(LogLevel::INFO, message);
    }

    void Warning(const std::string& message) {
        Log(LogLevel::WARNING, message);
    }

    void Error(const std::string& message) {
        Log(LogLevel::ERROR, message);
    }

    void Debug(const std::string& message) {
        Log(LogLevel::DEBUG, message);
    }

    std::vector<std::string> GetRecentLogs(size_t count = 100) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<std::string> result;
        size_t start_index = 0;
        
        if (log_buffer_.size() > count) {
            start_index = log_buffer_.size() - count;
        }
        
        for (size_t i = start_index; i < log_buffer_.size(); ++i) {
            result.push_back(log_buffer_[i]);
        }
        
        return result;
    }

    size_t GetTotalLogsCount() {
        std::lock_guard<std::mutex> lock(mutex_);
        return log_buffer_.size();
    }

private:
    Logger() {
        std::filesystem::create_directories("backend/logs");
        
        log_file_.open("backend/logs/events.log", std::ios::out | std::ios::app);
        
        if (!log_file_.is_open()) {
            log_file_.open("backend/logs/events.log", std::ios::out);
        }
        
        if (log_file_.is_open()) {
            // Записываем первое сообщение напрямую, чтобы избежать рекурсии
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;
            
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
            ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
            ss << " [INFO] Logger initialized";
            
            std::string log_line = ss.str();
            log_file_ << log_line << std::endl;
            log_file_.flush();
            
            log_buffer_.push_back(log_line);
        }
    }

    ~Logger() {
        if (log_file_.is_open()) {
            // Записываем сообщение о завершении напрямую
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;
            
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
            ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
            ss << " [INFO] Logger shutdown";
            
            log_file_ << ss.str() << std::endl;
            log_file_.flush();
            log_file_.close();
        }
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string LevelToString(LogLevel level) {
        switch (level) {
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::DEBUG: return "DEBUG";
            default: return "UNKNOWN";
        }
    }

    std::ofstream log_file_;
    std::mutex mutex_;
    std::deque<std::string> log_buffer_;
};

// Удобные макросы для логирования
#define LOG_INFO(msg) logger::Logger::Instance().Info(msg)
#define LOG_WARNING(msg) logger::Logger::Instance().Warning(msg)
#define LOG_ERROR(msg) logger::Logger::Instance().Error(msg)
#define LOG_DEBUG(msg) logger::Logger::Instance().Debug(msg)

} // namespace logger
