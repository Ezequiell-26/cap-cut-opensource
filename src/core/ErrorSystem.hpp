#pragma once

#include <string>
#include <cstdint>
#include <source_location>
#include <variant>
#include <vector>
#include <unordered_map>

namespace ccos::core {

// ============================================================================
// CATEGORÍAS DE ERROR
// ============================================================================

enum class ErrorCategory : uint8_t {
    NONE = 0,
    CORE,
    PROJECT,
    TIMELINE,
    MEDIA,
    RENDER,
    AUDIO,
    EFFECTS,
    TEXT,
    PLUGIN,
    AI,
    NETWORK,
    FILESYSTEM,
    SECURITY,
    VALIDATION,
    SYSTEM
};

inline const char* ErrorCategoryToString(ErrorCategory category) {
    switch (category) {
        case ErrorCategory::NONE: return "NONE";
        case ErrorCategory::CORE: return "CORE";
        case ErrorCategory::PROJECT: return "PROJECT";
        case ErrorCategory::TIMELINE: return "TIMELINE";
        case ErrorCategory::MEDIA: return "MEDIA";
        case ErrorCategory::RENDER: return "RENDER";
        case ErrorCategory::AUDIO: return "AUDIO";
        case ErrorCategory::EFFECTS: return "EFFECTS";
        case ErrorCategory::TEXT: return "TEXT";
        case ErrorCategory::PLUGIN: return "PLUGIN";
        case ErrorCategory::AI: return "AI";
        case ErrorCategory::NETWORK: return "NETWORK";
        case ErrorCategory::FILESYSTEM: return "FILESYSTEM";
        case ErrorCategory::SECURITY: return "SECURITY";
        case ErrorCategory::VALIDATION: return "VALIDATION";
        case ErrorCategory::SYSTEM: return "SYSTEM";
        default: return "UNKNOWN";
    }
}

// ============================================================================
// CÓDIGOS DE ERROR ESPECÍFICOS
// ============================================================================

enum class ErrorCode : uint32_t {
    // CORE (0-99)
    OK = 0,
    UNKNOWN_ERROR = 1,
    INTERNAL_ERROR = 2,
    NOT_IMPLEMENTED = 3,
    INVALID_STATE = 4,
    TIMEOUT = 5,
    CANCELLED = 6,
    
    // PROJECT (100-199)
    PROJECT_NOT_FOUND = 100,
    PROJECT_CORRUPTED = 101,
    PROJECT_VERSION_UNSUPPORTED = 102,
    PROJECT_SAVE_FAILED = 103,
    PROJECT_LOAD_FAILED = 104,
    PROJECT_INVALID_SCHEMA = 105,
    
    // TIMELINE (200-299)
    TIMELINE_INVALID_OPERATION = 200,
    TIMELINE_CLIP_NOT_FOUND = 201,
    TIMELINE_TRACK_NOT_FOUND = 202,
    TIMELINE_OVERLAP_DETECTED = 203,
    TIMELINE_INVALID_TIME = 204,
    TIMELINE_SPLIT_FAILED = 205,
    TIMELINE_TRIM_FAILED = 206,
    
    // MEDIA (300-399)
    MEDIA_NOT_FOUND = 300,
    MEDIA_INVALID_FORMAT = 301,
    MEDIA_DECODING_FAILED = 302,
    MEDIA_PROBE_FAILED = 303,
    MEDIA_IMPORT_FAILED = 304,
    MEDIA_FILE_TOO_LARGE = 305,
    MEDIA_CODEC_UNSUPPORTED = 306,
    
    // RENDER (400-499)
    RENDER_FAILED = 400,
    RENDER_CANCELLED = 401,
    RENDER_OUT_OF_MEMORY = 402,
    RENDER_GRAPH_INVALID = 403,
    RENDER_ENCODER_NOT_FOUND = 404,
    RENDER_TIMEOUT = 405,
    
    // AUDIO (500-599)
    AUDIO_DEVICE_NOT_FOUND = 500,
    AUDIO_PLAYBACK_FAILED = 501,
    AUDIO_RECORDING_FAILED = 502,
    AUDIO_MIXING_FAILED = 503,
    AUDIO_EFFECT_FAILED = 504,
    AUDIO_SYNC_LOST = 505,
    
    // PLUGIN (600-699)
    PLUGIN_NOT_FOUND = 600,
    PLUGIN_INCOMPATIBLE = 601,
    PLUGIN_DENIED = 602,
    PLUGIN_LOAD_FAILED = 603,
    PLUGIN_CRASHED = 604,
    PLUGIN_PERMISSION_DENIED = 605,
    
    // AI (700-799)
    AI_SERVICE_UNAVAILABLE = 700,
    AI_TIMEOUT = 701,
    AI_INVALID_REQUEST = 702,
    AI_RATE_LIMIT_EXCEEDED = 703,
    AI_MODEL_NOT_FOUND = 704,
    AI_INVALID_RESPONSE = 705,
    
    // NETWORK (800-899)
    NETWORK_UNAVAILABLE = 800,
    NETWORK_TIMEOUT = 801,
    NETWORK_CONNECTION_REFUSED = 802,
    NETWORK_SSL_ERROR = 803,
    NETWORK_HTTP_ERROR = 804,
    
    // FILESYSTEM (900-999)
    FILE_NOT_FOUND = 900,
    FILE_ACCESS_DENIED = 901,
    FILE_ALREADY_EXISTS = 902,
    FILE_WRITE_FAILED = 903,
    FILE_READ_FAILED = 904,
    DIRECTORY_NOT_FOUND = 905,
    DISK_FULL = 906,
    
    // SECURITY (1000-1099)
    SECURITY_VIOLATION = 1000,
    PERMISSION_DENIED = 1001,
    INVALID_CREDENTIALS = 1002,
    SIGNATURE_MISMATCH = 1003,
    PATH_TRAVERSAL_DETECTED = 1004,
    COMMAND_INJECTION_DETECTED = 1005,
    
    // VALIDATION (1100-1199)
    VALIDATION_FAILED = 1100,
    INVALID_ARGUMENT = 1101,
    NULL_POINTER = 1102,
    OUT_OF_RANGE = 1103,
    TYPE_MISMATCH = 1104
};

inline const char* ErrorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::OK: return "OK";
        case ErrorCode::UNKNOWN_ERROR: return "UNKNOWN_ERROR";
        case ErrorCode::INTERNAL_ERROR: return "INTERNAL_ERROR";
        case ErrorCode::NOT_IMPLEMENTED: return "NOT_IMPLEMENTED";
        case ErrorCode::INVALID_STATE: return "INVALID_STATE";
        case ErrorCode::TIMEOUT: return "TIMEOUT";
        case ErrorCode::CANCELLED: return "CANCELLED";
        
        case ErrorCode::PROJECT_NOT_FOUND: return "PROJECT_NOT_FOUND";
        case ErrorCode::PROJECT_CORRUPTED: return "PROJECT_CORRUPTED";
        case ErrorCode::PROJECT_VERSION_UNSUPPORTED: return "PROJECT_VERSION_UNSUPPORTED";
        
        case ErrorCode::TIMELINE_INVALID_OPERATION: return "TIMELINE_INVALID_OPERATION";
        case ErrorCode::TIMELINE_CLIP_NOT_FOUND: return "TIMELINE_CLIP_NOT_FOUND";
        case ErrorCode::TIMELINE_OVERLAP_DETECTED: return "TIMELINE_OVERLAP_DETECTED";
        
        case ErrorCode::MEDIA_NOT_FOUND: return "MEDIA_NOT_FOUND";
        case ErrorCode::MEDIA_INVALID_FORMAT: return "MEDIA_INVALID_FORMAT";
        case ErrorCode::MEDIA_DECODING_FAILED: return "MEDIA_DECODING_FAILED";
        
        case ErrorCode::RENDER_FAILED: return "RENDER_FAILED";
        case ErrorCode::RENDER_CANCELLED: return "RENDER_CANCELLED";
        case ErrorCode::RENDER_OUT_OF_MEMORY: return "RENDER_OUT_OF_MEMORY";
        
        case ErrorCode::AUDIO_DEVICE_NOT_FOUND: return "AUDIO_DEVICE_NOT_FOUND";
        case ErrorCode::AUDIO_PLAYBACK_FAILED: return "AUDIO_PLAYBACK_FAILED";
        case ErrorCode::AUDIO_MIXING_FAILED: return "AUDIO_MIXING_FAILED";
        
        case ErrorCode::PLUGIN_NOT_FOUND: return "PLUGIN_NOT_FOUND";
        case ErrorCode::PLUGIN_INCOMPATIBLE: return "PLUGIN_INCOMPATIBLE";
        case ErrorCode::PLUGIN_DENIED: return "PLUGIN_DENIED";
        
        case ErrorCode::AI_SERVICE_UNAVAILABLE: return "AI_SERVICE_UNAVAILABLE";
        case ErrorCode::AI_TIMEOUT: return "AI_TIMEOUT";
        case ErrorCode::AI_INVALID_REQUEST: return "AI_INVALID_REQUEST";
        
        case ErrorCode::NETWORK_UNAVAILABLE: return "NETWORK_UNAVAILABLE";
        case ErrorCode::NETWORK_TIMEOUT: return "NETWORK_TIMEOUT";
        
        case ErrorCode::FILE_NOT_FOUND: return "FILE_NOT_FOUND";
        case ErrorCode::FILE_ACCESS_DENIED: return "FILE_ACCESS_DENIED";
        case ErrorCode::DISK_FULL: return "DISK_FULL";
        
        case ErrorCode::SECURITY_VIOLATION: return "SECURITY_VIOLATION";
        case ErrorCode::PERMISSION_DENIED: return "PERMISSION_DENIED";
        case ErrorCode::PATH_TRAVERSAL_DETECTED: return "PATH_TRAVERSAL_DETECTED";
        case ErrorCode::COMMAND_INJECTION_DETECTED: return "COMMAND_INJECTION_DETECTED";
        
        case ErrorCode::VALIDATION_FAILED: return "VALIDATION_FAILED";
        case ErrorCode::INVALID_ARGUMENT: return "INVALID_ARGUMENT";
        case ErrorCode::NULL_POINTER: return "NULL_POINTER";
        
        default: return "UNKNOWN_CODE";
    }
}

// ============================================================================
// DETALLES DEL ERROR
// ============================================================================

struct ErrorDetails {
    std::string key;
    std::variant<std::string, int64_t, double, bool> value;
    
    ErrorDetails(const std::string& k, const std::string& v) : key(k), value(v) {}
    ErrorDetails(const std::string& k, int64_t v) : key(k), value(v) {}
    ErrorDetails(const std::string& k, double v) : key(k), value(v) {}
    ErrorDetails(const std::string& k, bool v) : key(k), value(v) {}
};

// ============================================================================
// CLASE ERROR
// ============================================================================

class Error {
public:
    Error() 
        : code_(ErrorCode::OK)
        , category_(ErrorCategory::NONE)
        , recoverable_(true)
        , retryable_(false) {}
    
    Error(ErrorCode code, 
          ErrorCategory category,
          const std::string& message,
          const std::source_location& location = std::source_location::current())
        : code_(code)
        , category_(category)
        , message_(message)
        , recoverable_(isRecoverable(code))
        , retryable_(isRetryable(code))
        , file_(location.file_name())
        , line_(location.line())
        , function_(location.function_name()) {}
    
    static Error OK() {
        return Error();
    }
    
    // Getters
    ErrorCode code() const { return code_; }
    ErrorCategory category() const { return category_; }
    const std::string& message() const { return message_; }
    const std::string& userMessage() const { return userMessage_.empty() ? message_ : userMessage_; }
    const std::string& developerMessage() const { return developerMessage_; }
    bool isRecoverable() const { return recoverable_; }
    bool isRetryable() const { return retryable_; }
    const std::string& file() const { return file_; }
    int line() const { return line_; }
    const std::string& function() const { return function_; }
    const std::vector<ErrorDetails>& details() const { return details_; }
    
    // Setters con fluent interface
    Error& withUserMessage(const std::string& msg) {
        userMessage_ = msg;
        return *this;
    }
    
    Error& withDeveloperMessage(const std::string& msg) {
        developerMessage_ = msg;
        return *this;
    }
    
    Error& withDetail(const std::string& key, const std::string& value) {
        details_.emplace_back(key, value);
        return *this;
    }
    
    Error& withDetail(const std::string& key, int64_t value) {
        details_.emplace_back(key, value);
        return *this;
    }
    
    Error& withDetail(const std::string& key, double value) {
        details_.emplace_back(key, value);
        return *this;
    }
    
    Error& asNonRecoverable() {
        recoverable_ = false;
        return *this;
    }
    
    Error& asRetryable() {
        retryable_ = true;
        return *this;
    }
    
    // Conversión a bool (true si hay error)
    explicit operator bool() const {
        return code_ != ErrorCode::OK;
    }
    
    bool hasError() const {
        return code_ != ErrorCode::OK;
    }
    
    // Formateo completo para logging
    std::string toString() const {
        if (!hasError()) return "No error";
        
        std::string result = "[" + std::string(ErrorCategoryToString(category_)) + "] ";
        result += ErrorCodeToString(code_) + ": " + message_;
        
        if (!details_.empty()) {
            result += " | Details: ";
            for (size_t i = 0; i < details_.size(); ++i) {
                if (i > 0) result += ", ";
                result += details_[i].key + "=";
                
                // Convertir variant a string
                if (auto strVal = std::get_if<std::string>(&details_[i].value)) {
                    result += *strVal;
                } else if (auto intVal = std::get_if<int64_t>(&details_[i].value)) {
                    result += std::to_string(*intVal);
                } else if (auto dblVal = std::get_if<double>(&details_[i].value)) {
                    result += std::to_string(*dblVal);
                } else if (auto boolVal = std::get_if<bool>(&details_[i].value)) {
                    result += *boolVal ? "true" : "false";
                }
            }
        }
        
        result += " [" + file_ + ":" + std::to_string(line_) + " in " + function_ + "]";
        return result;
    }

private:
    ErrorCode code_;
    ErrorCategory category_;
    std::string message_;
    std::string userMessage_;
    std::string developerMessage_;
    bool recoverable_;
    bool retryable_;
    std::string file_;
    int line_;
    std::string function_;
    std::vector<ErrorDetails> details_;
    
    static bool isRecoverable(ErrorCode code) {
        switch (code) {
            case ErrorCode::PROJECT_CORRUPTED:
            case ErrorCode::MEDIA_INVALID_FORMAT:
            case ErrorCode::SECURITY_VIOLATION:
            case ErrorCode::PERMISSION_DENIED:
                return false;
            default:
                return true;
        }
    }
    
    static bool isRetryable(ErrorCode code) {
        switch (code) {
            case ErrorCode::TIMEOUT:
            case ErrorCode::NETWORK_UNAVAILABLE:
            case ErrorCode::NETWORK_TIMEOUT:
            case ErrorCode::AI_SERVICE_UNAVAILABLE:
            case ErrorCode::AI_TIMEOUT:
            case ErrorCode::DISK_FULL:
                return true;
            default:
                return false;
        }
    }
};

// ============================================================================
// RESULT<T> - TIPO DE RETORNO SEGURO
// ============================================================================

template<typename T>
class Result {
public:
    Result(T value) 
        : hasValue_(true)
        , value_(std::move(value))
        , error_() {}
    
    Result(Error error)
        : hasValue_(false)
        , value_()
        , error_(std::move(error)) {}
    
    static Result OK(T value) {
        return Result(std::move(value));
    }
    
    static Result Err(Error error) {
        return Result(std::move(error));
    }
    
    bool hasValue() const { return hasValue_; }
    bool hasError() const { return !hasValue_; }
    
    T& value() { 
        if (!hasValue_) throw std::runtime_error("Accessing value of error result");
        return value_; 
    }
    
    const T& value() const { 
        if (!hasValue_) throw std::runtime_error("Accessing value of error result");
        return value_; 
    }
    
    T valueOr(T defaultValue) const {
        return hasValue_ ? value_ : defaultValue;
    }
    
    Error& error() { 
        if (hasValue_) throw std::runtime_error("Accessing error of success result");
        return error_; 
    }
    
    const Error& error() const { 
        if (hasValue_) throw std::runtime_error("Accessing error of success result");
        return error_; 
    }
    
    explicit operator bool() const { return hasValue_; }

private:
    bool hasValue_;
    T value_;
    Error error_;
};

// Specialization for void
template<>
class Result<void> {
public:
    Result() : hasValue_(true), error_() {}
    Result(Error error) : hasValue_(false), error_(std::move(error)) {}
    
    static Result OK() { return Result(); }
    static Result Err(Error error) { return Result(std::move(error)); }
    
    bool hasValue() const { return hasValue_; }
    bool hasError() const { return !hasValue_; }
    
    Error& error() { 
        if (hasValue_) throw std::runtime_error("Accessing error of success result");
        return error_; 
    }
    
    const Error& error() const { 
        if (hasValue_) throw std::runtime_error("Accessing error of success result");
        return error_; 
    }
    
    explicit operator bool() const { return hasValue_; }

private:
    bool hasValue_;
    Error error_;
};

// ============================================================================
// MACROS DE UTILIDAD
// ============================================================================

#define CCOS_RETURN_IF_ERROR(expr) \
    do { \
        auto result = (expr); \
        if (result.hasError()) { \
            return result; \
        } \
    } while (0)

#define CCOS_THROW_IF_ERROR(expr) \
    do { \
        auto result = (expr); \
        if (result.hasError()) { \
            throw std::runtime_error(result.error().toString()); \
        } \
    } while (0)

#define CCOS_CREATE_ERROR(code, category, msg) \
    ccos::core::Error(ccos::core::ErrorCode::code, ccos::core::ErrorCategory::category, msg)

} // namespace ccos::core
