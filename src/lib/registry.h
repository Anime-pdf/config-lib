#ifndef CONFIG_REGISTRY_H
#define CONFIG_REGISTRY_H

#include <memory>
#include <mutex>
#include <ranges>
#include <string>
#include <unordered_map>

#include "variable.h"

class CConfigRegistry {
    std::unordered_map<std::string, std::unique_ptr<IConfigVariableBase> > m_Variables;
    mutable std::mutex m_Mutex;

    CConfigRegistry() = default;

public:
    static CConfigRegistry &Instance() {
        static CConfigRegistry instance;
        return instance;
    }

    template<typename T>
    bool Register(CConfigVariable<T> &&var) {
        std::lock_guard lock(m_Mutex);
        const std::string name(var.Name());

        if (m_Variables.contains(name))
            return false;

        m_Variables[name] = std::make_unique<CConfigVariable<T> >(std::move(var));
        return true;
    }

    template<typename T>
    std::optional<T> Get(const std::string &name) const {
        std::lock_guard lock(m_Mutex);
        const auto it = m_Variables.find(name);
        if (it == m_Variables.end())
            return std::nullopt;

        auto *wrapper = dynamic_cast<CConfigVariable<T> *>(it->second.get());
        if (!wrapper)
            return std::nullopt;

        return wrapper->Value();
    }

    template<typename T>
    std::optional<T> Type(const std::string &name) const {
        std::lock_guard lock(m_Mutex);
        const auto it = m_Variables.find(name);
        if (it == m_Variables.end())
            return std::nullopt;

        auto *wrapper = dynamic_cast<CConfigVariable<T> *>(it->second.get());
        if (!wrapper)
            return std::nullopt;

        return wrapper->Value();
    }

    std::expected<void, std::string> Set(const std::string &name, const std::string &value) {
        std::lock_guard lock(m_Mutex);
        const auto it = m_Variables.find(name);
        if (it == m_Variables.end())
            return std::unexpected("Variable '" + name + "' not found");

        return it->second->TrySet(value);
    }

    bool Exists(const std::string &name) const {
        std::lock_guard lock(m_Mutex);
        return m_Variables.contains(name);
    }

    std::optional<std::string> GetAsString(const std::string &name) const {
        std::lock_guard lock(m_Mutex);
        const auto it = m_Variables.find(name);
        if (it == m_Variables.end())
            return std::nullopt;
        return it->second->ValueAsString();
    }

    bool Reset(const std::string &name) {
        std::lock_guard lock(m_Mutex);
        const auto it = m_Variables.find(name);
        if (it == m_Variables.end())
            return false;
        it->second->Reset();
        return true;
    }

    std::vector<std::string> ListAll() const {
        std::lock_guard lock(m_Mutex);
        std::vector<std::string> names;
        names.reserve(m_Variables.size());
        for (const auto &name: m_Variables | std::views::keys)
            names.push_back(name);
        return names;
    }

    struct VariableInfo {
        std::string name;
        std::string type;
        std::string value;
        std::string default_value;
        std::optional<std::string> description;
    };

    std::optional<VariableInfo> GetInfo(const std::string &name) const {
        std::lock_guard lock(m_Mutex);
        const auto it = m_Variables.find(name);
        if (it == m_Variables.end())
            return std::nullopt;

        VariableInfo info;
        info.name = std::string(it->second->Name());
        info.type = it->second->Type().name();
        info.value = it->second->ValueAsString();
        info.default_value = it->second->DefaultValueAsString();
        if (const auto desc = it->second->Description())
            info.description = std::string(*desc);

        return info;
    }
};

inline CConfigRegistry &Config() { return CConfigRegistry::Instance(); }

#define CONFIG_STRING(name, defaultValue, validators) CConfigRegistry::Instance().Register<std::string>(CConfigVariable<std::string>(name, defaultValue, validators))
#define CONFIG_INT(name, defaultValue, validators) CConfigRegistry::Instance().Register<int>(CConfigVariable<int>(name, defaultValue, validators))
#define CONFIG_FLOAT(name, defaultValue, validators) CConfigRegistry::Instance().Register<float>(CConfigVariable<float>(name, defaultValue, validators))

#endif // CONFIG_REGISTRY_H
