#include "Engine/Core/CVar.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    CVarRegistry& CVarRegistry::Get() {
        static CVarRegistry instance;
        return instance;
    }

    void CVarRegistry::RegisterCVar(const std::string& name, int defaultValue, const std::string& description) {
        if (m_CVars.find(name) == m_CVars.end()) {
            m_CVars[name] = {{CVarType::INT, defaultValue}, description};
        }
    }

    void CVarRegistry::RegisterCVar(const std::string& name, float defaultValue, const std::string& description) {
        if (m_CVars.find(name) == m_CVars.end()) {
            m_CVars[name] = {{CVarType::FLOAT, defaultValue}, description};
        }
    }

    void CVarRegistry::RegisterCVar(const std::string& name, const std::string& defaultValue, const std::string& description) {
        if (m_CVars.find(name) == m_CVars.end()) {
            m_CVars[name] = {{CVarType::STRING, defaultValue}, description};
        }
    }

    void CVarRegistry::RegisterCVar(const std::string& name, bool defaultValue, const std::string& description) {
        if (m_CVars.find(name) == m_CVars.end()) {
            m_CVars[name] = {{CVarType::BOOL, defaultValue}, description};
        }
    }

    int CVarRegistry::GetCVarInt(const std::string& name, int fallback) const {
        auto it = m_CVars.find(name);
        if (it != m_CVars.end() && it->second.value.type == CVarType::INT) {
            return std::get<int>(it->second.value.value);
        }
        return fallback;
    }

    float CVarRegistry::GetCVarFloat(const std::string& name, float fallback) const {
        auto it = m_CVars.find(name);
        if (it != m_CVars.end() && it->second.value.type == CVarType::FLOAT) {
            return std::get<float>(it->second.value.value);
        }
        return fallback;
    }

    std::string CVarRegistry::GetCVarString(const std::string& name, const std::string& fallback) const {
        auto it = m_CVars.find(name);
        if (it != m_CVars.end() && it->second.value.type == CVarType::STRING) {
            return std::get<std::string>(it->second.value.value);
        }
        return fallback;
    }

    bool CVarRegistry::GetCVarBool(const std::string& name, bool fallback) const {
        auto it = m_CVars.find(name);
        if (it != m_CVars.end() && it->second.value.type == CVarType::BOOL) {
            return std::get<bool>(it->second.value.value);
        }
        return fallback;
    }

    void CVarRegistry::SetCVar(const std::string& name, int value) {
        auto it = m_CVars.find(name);
        if (it != m_CVars.end() && it->second.value.type == CVarType::INT) {
            it->second.value.value = value;
        } else {
            VECTOR_LOG_WARN("Attempted to set non-existent or wrongly typed CVar (int): " + name);
        }
    }

    void CVarRegistry::SetCVar(const std::string& name, float value) {
        auto it = m_CVars.find(name);
        if (it != m_CVars.end() && it->second.value.type == CVarType::FLOAT) {
            it->second.value.value = value;
        } else {
            VECTOR_LOG_WARN("Attempted to set non-existent or wrongly typed CVar (float): " + name);
        }
    }

    void CVarRegistry::SetCVar(const std::string& name, const std::string& value) {
        auto it = m_CVars.find(name);
        if (it != m_CVars.end() && it->second.value.type == CVarType::STRING) {
            it->second.value.value = value;
        } else {
            VECTOR_LOG_WARN("Attempted to set non-existent or wrongly typed CVar (string): " + name);
        }
    }

    void CVarRegistry::SetCVar(const std::string& name, bool value) {
        auto it = m_CVars.find(name);
        if (it != m_CVars.end() && it->second.value.type == CVarType::BOOL) {
            it->second.value.value = value;
        } else {
            VECTOR_LOG_WARN("Attempted to set non-existent or wrongly typed CVar (bool): " + name);
        }
    }

} // namespace VECTOR
