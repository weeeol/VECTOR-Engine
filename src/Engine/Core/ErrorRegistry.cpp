#include "Engine/Core/ErrorRegistry.hpp"

namespace VECTOR {

    ErrorRegistry& ErrorRegistry::Get() {
        static ErrorRegistry instance;
        return instance;
    }

    ErrorRegistry::ErrorRegistry() {
        // Register default mappings
        RegisterError(ErrorCode::SUCCESS, "Success");
        RegisterError(ErrorCode::UNKNOWN_ERROR, "Unknown Error");
        RegisterError(ErrorCode::GRAPHICS_API_ERROR, "Graphics API Error");
        RegisterError(ErrorCode::GRAPHICS_OUT_OF_MEMORY, "Graphics Out of Memory");
        RegisterError(ErrorCode::GRAPHICS_DEVICE_LOST, "Graphics Device Lost");
        RegisterError(ErrorCode::GRAPHICS_INVALID_ARGUMENT, "Graphics Invalid Argument");
        RegisterError(ErrorCode::RESOURCE_NOT_FOUND, "Resource Not Found");
        RegisterError(ErrorCode::RESOURCE_LOAD_FAILED, "Resource Load Failed");
        RegisterError(ErrorCode::CORE_INITIALIZATION_FAILED, "Core Initialization Failed");
    }

    void ErrorRegistry::RegisterError(ErrorCode code, const std::string& message) {
        m_ErrorMessages[code] = message;
    }

    void ErrorRegistry::RegisterAPIError(int32_t apiCode, const std::string& apiName, const std::string& message) {
        m_APIErrors[apiName][apiCode] = message;
    }

    std::string ErrorRegistry::GetErrorMessage(ErrorCode code) const {
        auto it = m_ErrorMessages.find(code);
        if (it != m_ErrorMessages.end()) {
            return it->second;
        }
        return "Unknown Error Code";
    }

    std::string ErrorRegistry::GetAPIErrorMessage(int32_t apiCode, const std::string& apiName) const {
        auto apiIt = m_APIErrors.find(apiName);
        if (apiIt != m_APIErrors.end()) {
            auto msgIt = apiIt->second.find(apiCode);
            if (msgIt != apiIt->second.end()) {
                return msgIt->second;
            }
        }
        return "Unknown " + apiName + " Error Code: " + std::to_string(apiCode);
    }

} // namespace VECTOR
