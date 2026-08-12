#pragma once

#include <string>
#include <unordered_map>
#include <variant>

namespace VECTOR {

    enum class CVarType {
        INT,
        FLOAT,
        STRING,
        BOOL
    };

    struct CVarValue {
        CVarType type;
        std::variant<int, float, std::string, bool> value;
    };

    class CVarRegistry {
    public:
        static CVarRegistry& Get();

        void RegisterCVar(const std::string& name, int defaultValue, const std::string& description = "");
        void RegisterCVar(const std::string& name, float defaultValue, const std::string& description = "");
        void RegisterCVar(const std::string& name, const std::string& defaultValue, const std::string& description = "");
        void RegisterCVar(const std::string& name, bool defaultValue, const std::string& description = "");

        int GetCVarInt(const std::string& name, int fallback = 0) const;
        float GetCVarFloat(const std::string& name, float fallback = 0.0f) const;
        std::string GetCVarString(const std::string& name, const std::string& fallback = "") const;
        bool GetCVarBool(const std::string& name, bool fallback = false) const;

        void SetCVar(const std::string& name, int value);
        void SetCVar(const std::string& name, float value);
        void SetCVar(const std::string& name, const std::string& value);
        void SetCVar(const std::string& name, bool value);

    private:
        CVarRegistry() = default;

        struct CVarEntry {
            CVarValue value;
            std::string description;
        };

        std::unordered_map<std::string, CVarEntry> m_CVars;
    };

} // namespace VECTOR
