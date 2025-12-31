#ifndef CONFIG_REGISTRY_H
#define CONFIG_REGISTRY_H

#include <memory>
#include <mutex>
#include <ranges>
#include <string>
#include <unordered_map>
#include <fstream>

#include <config/external/json.hpp>

#include "variable.h"

using namespace nlohmann;

class CConfigRegistry {
    std::unordered_map<std::string, std::unique_ptr<IConfigVariableBase> > m_Variables;
    mutable std::mutex m_Mutex;
    std::string m_ConfigPath;

    CConfigRegistry() = default;

    // json helpers

    json &GetNestedJson(json &root, const std::string &path) {
        size_t start = 0;
        size_t end = path.find('.');
        json *current = &root;

        while (end != std::string::npos) {
            std::string part = path.substr(start, end - start);
            if (!current->contains(part) || !(*current)[part].is_object()) {
                (*current)[part] = json::object();
            }
            current = &(*current)[part];
            start = end + 1;
            end = path.find('.', start);
        }

        return *current;
    }

    void SetNestedValue(json &root, const std::string &path, const json &value) {
        size_t lastDot = path.rfind('.');
        if (lastDot == std::string::npos) {
            root[path] = value;
        } else {
            std::string parentPath = path.substr(0, lastDot);
            std::string key = path.substr(lastDot + 1);
            json &parent = GetNestedJson(root, parentPath);
            parent[key] = value;
        }
    }

    std::optional<json> GetNestedValue(const json &root, const std::string &path) const {
        size_t start = 0;
        size_t end = path.find('.');
        const json *current = &root;

        while (end != std::string::npos) {
            std::string part = path.substr(start, end - start);
            if (!current->contains(part))
                return std::nullopt;
            current = &(*current)[part];
            start = end + 1;
            end = path.find('.', start);
        }

        std::string finalKey = path.substr(start);
        if (!current->contains(finalKey))
            return std::nullopt;

        return (*current)[finalKey];
    }

public:
    static CConfigRegistry &Instance() {
        static CConfigRegistry instance;
        return instance;
    }

    void SetConfigPath(const std::string &path) {
        std::lock_guard lock(m_Mutex);
        m_ConfigPath = path;
    }

    std::string GetConfigPath() const {
        std::lock_guard lock(m_Mutex);
        return m_ConfigPath;
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

    void ResetAll(const std::string &name) {
        std::lock_guard lock(m_Mutex);
        for (const auto &var: m_Variables | std::views::values) {
            var->Reset();
        }
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
        bool readonly = false;
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
        info.readonly = it->second->ReadOnly();
        info.name = std::string(it->second->Name());
        info.type = it->second->TypeString();
        info.value = it->second->ValueAsString();
        info.default_value = it->second->DefaultValueAsString();
        if (const auto desc = it->second->Description())
            info.description = std::string(*desc);

        return info;
    }

    // serialization

    std::expected<void, std::string> SaveToFile(const std::string &filepath) {
        std::lock_guard lock(m_Mutex);

        try {
            json root = json::object();

            for (const auto &[name, var]: m_Variables) {
                SetNestedValue(root, name, var->ValueAsJson());
            }

            std::ofstream file(filepath);
            if (!file.is_open())
                return std::unexpected("Failed to open file for writing: " + filepath);

            file << root.dump(4);
            file.close();

            return {};
        } catch (const std::exception &e) {
            return std::unexpected("Error saving config: " + std::string(e.what()));
        }
    }

    std::expected<void, std::string> Save() {
        if (m_ConfigPath.empty())
            return std::unexpected("No config path set. Use SetConfigPath() first.");
        return SaveToFile(m_ConfigPath);
    }

    std::expected<void, std::string> LoadFromFile(const std::string &filepath) {
        std::lock_guard lock(m_Mutex);

        if (!std::filesystem::exists(filepath)) {
            return std::unexpected("File doesn't exist");
        }

        try {
            std::ifstream file(filepath);
            if (!file.is_open())
                return std::unexpected("Failed to open file for reading: " + filepath);

            json root;
            file >> root;
            file.close();

            std::vector<std::string> errors;

            for (const auto &[name, var]: m_Variables) {
                auto value = GetNestedValue(root, name);
                if (!value.has_value())
                    continue;

                auto result = var->TrySetJson(*value, true);
                if (!result.has_value()) {
                    errors.push_back(name + ": " + result.error());
                }
            }

            if (!errors.empty()) {
                std::string errorMsg = "Some variables failed to load:\n";
                for (const auto &err: errors)
                    errorMsg += " - " + err + "\n";
                return std::unexpected(errorMsg);
            }

            return {};
        } catch (const json::exception &e) {
            return std::unexpected("JSON parse error: " + std::string(e.what()));
        } catch (const std::exception &e) {
            return std::unexpected("Error loading config: " + std::string(e.what()));
        }
    }

    std::expected<void, std::string> Load() {
        if (m_ConfigPath.empty())
            return std::unexpected("No config path set. Use SetConfigPath() first.");
        return LoadFromFile(m_ConfigPath);
    }

    // config with metadata (descriptions, types, defaults)
    std::expected<void, std::string> ExportTemplate(const std::string &filepath) {
        std::lock_guard lock(m_Mutex);

        try {
            json root = json::object();

            for (const auto &[name, var]: m_Variables) {
                json varInfo = json::object();
                varInfo["readonly"] = var->ReadOnly();
                varInfo["value"] = var->ValueAsJson();
                varInfo["default"] = var->DefaultValueAsJson();
                varInfo["type"] = var->TypeString();
                if (auto desc = var->Description())
                    varInfo["description"] = std::string(*desc);

                SetNestedValue(root, name, varInfo);
            }

            std::ofstream file(filepath);
            if (!file.is_open())
                return std::unexpected("Failed to open file for writing: " + filepath);

            file << root.dump(4);
            file.close();

            return {};
        } catch (const std::exception &e) {
            return std::unexpected("Error exporting template: " + std::string(e.what()));
        }
    }
};

inline CConfigRegistry &Config() { return CConfigRegistry::Instance(); }

#define CONFIG_STRING(name, defaultValue, validators) CConfigRegistry::Instance().Register<std::string>(CConfigVariable<std::string>(name, defaultValue, validators))
#define CONFIG_STRING_READONLY(name, defaultValue, validators) CConfigRegistry::Instance().Register<std::string>(CConfigVariable<std::string>(name, defaultValue, validators, std::nullopt, true))
#define CONFIG_INT(name, defaultValue, validators) CConfigRegistry::Instance().Register<int>(CConfigVariable<int>(name, defaultValue, validators))
#define CONFIG_INT_READONLY(name, defaultValue, validators) CConfigRegistry::Instance().Register<int>(CConfigVariable<int>(name, defaultValue, validators, std::nullopt, true))
#define CONFIG_FLOAT(name, defaultValue, validators) CConfigRegistry::Instance().Register<float>(CConfigVariable<float>(name, defaultValue, validators))
#define CONFIG_FLOAT_READONLY(name, defaultValue, validators) CConfigRegistry::Instance().Register<float>(CConfigVariable<float>(name, defaultValue, validators, std::nullopt, true))
#define CONFIG_BOOLEAN(name, defaultValue, validators) CConfigRegistry::Instance().Register<bool>(CConfigVariable<bool>(name, defaultValue, validators))
#define CONFIG_BOOLEAN_READONLY(name, defaultValue, validators) CConfigRegistry::Instance().Register<bool>(CConfigVariable<bool>(name, defaultValue, validators, std::nullopt, true))

#endif // CONFIG_REGISTRY_H
