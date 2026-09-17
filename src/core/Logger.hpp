#pragma once

#include "ErrorSystem.hpp"
#include <string>
#include <cstdint>
#include <functional>
#include <memory>
#include <fstream>
#include <mutex>
#include <queue>
#include <chrono>

namespace ccos::core {

// ============================================================================
// NIVELES DE LOG
// ============================================================================

enum class LogLevel : uint8_t {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARNING = 3,
    ERROR = 4,
    CRITICAL = 5,
    FATAL = 6
};

inline const char* LogLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

// ============================================================================
// CATEGORÍAS DE LOG
// ============================================================================

enum class LogCategory : uint8_t {
    APP = 0,
    CORE,
    PROJECT,
    TIMELINE,
    MEDIA,
    RENDER,
    AUDIO,
    PLUGIN,
    AI,
    NETWORK,
    PERFORMANCE,
    SECURITY,
    UI,
    TEST
};

inline const char* LogCategoryToString(LogCategory category) {
    switch (category) {
        case LogCategory::APP: return "APP";
        case LogCategory::CORE: return "CORE";
        case LogCategory::PROJECT: return "PROJECT";
        case LogCategory::TIMELINE: return "TIMELINE";
        case LogCategory::MEDIA: return "MEDIA";
        case LogCategory::RENDER: return "RENDER";
        case LogCategory::AUDIO: return "AUDIO";
        case LogCategory::PLUGIN: return "PLUGIN";
        case LogCategory::AI: return "AI";
        case LogCategory::NETWORK: return "NETWORK";
        case LogCategory::PERFORMANCE: return "PERFORMANCE";
        case LogCategory::SECURITY: return "SECURITY";
        case LogCategory::UI: return "UI";
        case LogCategory::TEST: return "TEST";
        default: return "UNKNOWN";
    }
}

// ============================================================================
// ENTRADA DE LOG ESTRUCTURADO
// ============================================================================

struct LogEntry {
    using Timestamp = std::chrono::system_clock::time_point;
    
    Timestamp timestamp;
    LogLevel level;
    LogCategory category;
    std::string message;
    std::string file;
    int line;
    std::string function;
    std::string threadId;
    
    // Campos estructurados para análisis
    std::unordered_map<std::string, std::string> fields;
    
    LogEntry() 
        : timestamp(std::chrono::system_clock::now())
        , level(LogLevel::INFO)
        , category(LogCategory::APP)
        , line(0) {}
    
    std::string toString() const {
        // Formato: [TIMESTAMP] [LEVEL] [CATEGORY] [THREAD] MESSAGE {fields}
        auto time = std::chrono::system_clock::to_time_t(timestamp);
        char timeBuf[64];
        std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&time));
        
        std::string result = "[" + std::string(timeBuf) + "] ";
        result += "[" + std::string(LogLevelToString(level)) + "] ";
        result += "[" + std::string(LogCategoryToString(category)) + "] ";
        result += "[" + threadId + "] ";
        result += message;
        
        if (!fields.empty()) {
            result += " | {";
            bool first = true;
            for (const auto& [key, value] : fields) {
                if (!first) result += ", ";
                result += key + "=" + value;
                first = false;
            }
            result += "}";
        }
        
        result += " (" + file + ":" + std::to_string(line) + ")";
        return result;
    }
};

// ============================================================================
// SINK DE LOG (DESTINO)
// ============================================================================

class LogSink {
public:
    virtual ~LogSink() = default;
    virtual void write(const LogEntry& entry) = 0;
    virtual void flush() = 0;
};

// Console Sink
class ConsoleLogSink : public LogSink {
public:
    void write(const LogEntry& entry) override {
        std::cout << entry.toString() << std::endl;
    }
    
    void flush() override {
        std::cout.flush();
    }
};

// File Sink con rotación
class FileLogSink : public LogSink {
public:
    FileLogSink(const std::string& basePath, 
                size_t maxFileSize = 10 * 1024 * 1024, // 10MB
                int maxFiles = 5)
        : basePath_(basePath)
        , maxFileSize_(maxFileSize)
        , maxFiles_(maxFiles)
        , currentSize_(0)
        , fileIndex_(0) {
        openCurrentFile();
    }
    
    ~FileLogSink() {
        if (file_.is_open()) {
            file_.close();
        }
    }
    
    void write(const LogEntry& entry) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string line = entry.toString() + "\n";
        
        // Rotar si excede tamaño
        if (currentSize_ + line.size() > maxFileSize_) {
            rotate();
        }
        
        if (file_.is_open()) {
            file_ << line;
            currentSize_ += line.size();
        }
    }
    
    void flush() override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open()) {
            file_.flush();
        }
    }

private:
    std::string basePath_;
    size_t maxFileSize_;
    int maxFiles_;
    size_t currentSize_;
    int fileIndex_;
    std::ofstream file_;
    std::mutex mutex_;
    
    void openCurrentFile() {
        std::string filename = basePath_ + "." + std::to_string(fileIndex_) + ".log";
        file_.open(filename, std::ios::app);
        if (file_.is_open()) {
            // Obtener tamaño actual
            file_.seekg(0, std::ios::end);
            currentSize_ = file_.tellg();
            file_.seekg(0, std::ios::beg);
        }
    }
    
    void rotate() {
        if (file_.is_open()) {
            file_.close();
        }
        
        // Eliminar archivo más viejo
        std::string oldestFile = basePath_ + "." + std::to_string((fileIndex_ + 1) % maxFiles_) + ".log";
        std::remove(oldestFile.c_str());
        
        // Mover a siguiente índice
        fileIndex_ = (fileIndex_ + 1) % maxFiles_;
        openCurrentFile();
    }
};

// ============================================================================
// LOGGER PRINCIPAL
// ============================================================================

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }
    
    // Configurar nivel mínimo global
    void setMinLevel(LogLevel level) {
        minLevel_ = level;
    }
    
    // Agregar sink
    void addSink(std::shared_ptr<LogSink> sink) {
        sinks_.push_back(sink);
    }
    
    // Habilitar/deshabilitar categoría
    void setCategoryEnabled(LogCategory category, bool enabled) {
        categoryEnabled_[category] = enabled;
    }
    
    // Log directo
    void log(LogLevel level, 
             LogCategory category,
             const std::string& message,
             const std::source_location& location = std::source_location::current()) {
        
        if (level < minLevel_) return;
        if (!isCategoryEnabled(category)) return;
        
        LogEntry entry;
        entry.level = level;
        entry.category = category;
        entry.message = message;
        entry.file = location.file_name();
        entry.line = location.line();
        entry.function = location.function_name();
        entry.threadId = getThreadId();
        
        dispatch(entry);
    }
    
    // Log con campos estructurados
    void logWithFields(LogLevel level,
                       LogCategory category,
                       const std::string& message,
                       std::initializer_list<std::pair<const std::string, std::string>> fields,
                       const std::source_location& location = std::source_location::current()) {
        
        if (level < minLevel_) return;
        if (!isCategoryEnabled(category)) return;
        
        LogEntry entry;
        entry.level = level;
        entry.category = category;
        entry.message = message;
        entry.file = location.file_name();
        entry.line = location.line();
        entry.function = location.function_name();
        entry.threadId = getThreadId();
        
        for (const auto& [key, value] : fields) {
            entry.fields[key] = value;
        }
        
        dispatch(entry);
    }
    
    // Log de error tipificado
    void logError(const Error& error) {
        logWithFields(LogLevel::ERROR, 
                      error.category(),
                      error.message(),
                      {{"code", ErrorCodeToString(error.code())},
                       {"recoverable", error.isRecoverable() ? "true" : "false"},
                       {"retryable", error.isRetryable() ? "true" : "false"}},
                      std::source_location::current());
    }
    
    // Helpers por nivel
    #define DEFINE_LOG_LEVEL(level, name) \
    void name(LogCategory category, const std::string& msg, \
              const std::source_location& loc = std::source_location::current()) { \
        log(level, category, msg, loc); \
    } \
    void name##F(LogCategory category, const std::string& msg, \
                 std::initializer_list<std::pair<const std::string, std::string>> fields, \
                 const std::source_location& loc = std::source_location::current()) { \
        logWithFields(level, category, msg, fields, loc); \
    }
    
    DEFINE_LOG_LEVEL(LogLevel::TRACE, trace)
    DEFINE_LOG_LEVEL(LogLevel::DEBUG, debug)
    DEFINE_LOG_LEVEL(LogLevel::INFO, info)
    DEFINE_LOG_LEVEL(LogLevel::WARNING, warning)
    DEFINE_LOG_LEVEL(LogLevel::ERROR, error)
    DEFINE_LOG_LEVEL(LogLevel::CRITICAL, critical)
    DEFINE_LOG_LEVEL(LogLevel::FATAL, fatal)
    
    #undef DEFINE_LOG_LEVEL
    
    // Flush todos los sinks
    void flush() {
        for (auto& sink : sinks_) {
            sink->flush();
        }
    }

private:
    Logger() {
        // Configurar defaults
        setMinLevel(LogLevel::DEBUG);
        
        // Habilitar todas las categorías por defecto
        for (int i = 0; i <= static_cast<int>(LogCategory::TEST); ++i) {
            categoryEnabled_[static_cast<LogCategory>(i)] = true;
        }
        
        // Agregar console sink por defecto
        addSink(std::make_shared<ConsoleLogSink>());
    }
    
    bool isCategoryEnabled(LogCategory category) const {
        auto it = categoryEnabled_.find(category);
        return it == categoryEnabled_.end() || it->second;
    }
    
    void dispatch(const LogEntry& entry) {
        for (auto& sink : sinks_) {
            sink->write(entry);
        }
    }
    
    std::string getThreadId() const {
        // Implementación simple - en producción usar std::hash<std::thread::id>
        return "main"; // Simplificado para este ejemplo
    }
    
    LogLevel minLevel_;
    std::vector<std::shared_ptr<LogSink>> sinks_;
    std::unordered_map<LogCategory, bool> categoryEnabled_;
};

// ============================================================================
// MACROS DE LOGGING
// ============================================================================

#define CCOS_LOG_TRACE(cat, msg) \
    ccos::core::Logger::instance().trace(cat, msg)

#define CCOS_LOG_DEBUG(cat, msg) \
    ccos::core::Logger::instance().debug(cat, msg)

#define CCOS_LOG_INFO(cat, msg) \
    ccos::core::Logger::instance().info(cat, msg)

#define CCOS_LOG_WARNING(cat, msg) \
    ccos::core::Logger::instance().warning(cat, msg)

#define CCOS_LOG_ERROR(cat, msg) \
    ccos::core::Logger::instance().error(cat, msg)

#define CCOS_LOG_CRITICAL(cat, msg) \
    ccos::core::Logger::instance().critical(cat, msg)

#define CCOS_LOG_FATAL(cat, msg) \
    ccos::core::Logger::instance().fatal(cat, msg)

// Logging con campos estructurados
#define CCOS_LOG_INFO_F(cat, msg, fields) \
    ccos::core::Logger::instance().infoF(cat, msg, fields)

#define CCOS_LOG_ERROR_F(cat, msg, fields) \
    ccos::core::Logger::instance().errorF(cat, msg, fields)

// Logging de errores tipificados
#define CCOS_LOG_ERROR_OBJ(error) \
    ccos::core::Logger::instance().logError(error)

// Scoped logging para performance
class ScopedTimer {
public:
    ScopedTimer(LogCategory category, const std::string& operation)
        : category_(category)
        , operation_(operation)
        , start_(std::chrono::high_resolution_clock::now()) {}
    
    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
        
        CCOS_LOG_INFO_F(category_, operation_ + " completed", {
            {"duration_us", std::to_string(duration)}
        });
    }

private:
    LogCategory category_;
    std::string operation_;
    std::chrono::high_resolution_clock::time_point start_;
};

#define CCOS_SCOPED_TIMER(cat, op) \
    ccos::core::ScopedTimer timer(cat, op)

} // namespace ccos::core
