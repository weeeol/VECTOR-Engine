#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>

namespace VECTOR {

    // Define standard Error Codes
    enum class ErrorCode : uint32_t {
        SUCCESS = 0,
        UNKNOWN_ERROR = 1,
        
        // Graphics Errors
        GRAPHICS_API_ERROR = 1000,
        GRAPHICS_OUT_OF_MEMORY = 1001,
        GRAPHICS_DEVICE_LOST = 1002,
        GRAPHICS_INVALID_ARGUMENT = 1003,

        // Resource Errors
        RESOURCE_NOT_FOUND = 2000,
        RESOURCE_LOAD_FAILED = 2001,

        // Core Errors
        CORE_INITIALIZATION_FAILED = 3000
    };

    class ErrorRegistry {
    public:
        static ErrorRegistry& Get();

        // Map an ErrorCode to a readable string
        void RegisterError(ErrorCode code, const std::string& message);

        // Map a graphics API specific error code (like HRESULT or VkResult) to a readable string
        void RegisterAPIError(int32_t apiCode, const std::string& apiName, const std::string& message);

        std::string GetErrorMessage(ErrorCode code) const;
        std::string GetAPIErrorMessage(int32_t apiCode, const std::string& apiName) const;

    private:
        ErrorRegistry();

        std::unordered_map<ErrorCode, std::string> m_ErrorMessages;
        std::unordered_map<std::string, std::unordered_map<int32_t, std::string>> m_APIErrors;
    };

} // namespace VECTOR
