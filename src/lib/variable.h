#ifndef CONFIG_VARIABLE_H
#define CONFIG_VARIABLE_H

#include <string>
#include <optional>
#include <functional>
#include <expected>
#include <typeindex>
#include <utility>

// abstract class for typeless storage
class IConfigVariableBase {
public:
    virtual ~IConfigVariableBase() = default;

    virtual std::type_index Type() const = 0;
    virtual std::string ValueAsString() const = 0;
    virtual std::string DefaultValueAsString() const = 0;

    virtual std::string_view Name() const = 0;
    virtual std::optional<std::string_view> Description() const = 0;

    virtual std::expected<void, std::string> TrySet(const std::string &value) = 0;
    virtual void Reset() = 0;
};

template<typename T>
class CConfigVariable : public IConfigVariableBase {
    std::string m_Name;
    T m_Value;
    T m_DefaultValue;
    std::optional<std::string> m_Description;
    std::function<std::expected<T, std::string>(std::string)> m_Validator;

public:
    CConfigVariable(std::string Name,
                    T DefaultValue,
                    std::function<std::expected<T, std::string>(std::string)> Validator,
                    const std::optional<std::string> &Description = std::nullopt)
        : m_Name(std::move(Name)), m_Value(DefaultValue), m_DefaultValue(DefaultValue), m_Description(Description),
          m_Validator(std::move(Validator)) {
    }

    std::type_index Type() const override { return typeid(T); }

    std::string ValueAsString() const override {
        if constexpr (std::is_same_v<T, std::string>) {
            return m_Value;
        } else if constexpr (std::is_same_v<T, bool>) {
            return m_Value ? "true" : "false";
        } else {
            return std::to_string(m_Value);
        }
    }

    std::string DefaultValueAsString() const override {
        if constexpr (std::is_same_v<T, std::string>) {
            return m_DefaultValue;
        } else if constexpr (std::is_same_v<T, bool>) {
            return m_DefaultValue ? "true" : "false";
        } else {
            return std::to_string(m_DefaultValue);
        }
    }

    std::string_view Name() const override { return m_Name; }
    T Value() const { return m_Value; }
    T DefaultValue() const { return m_DefaultValue; }
    std::optional<std::string_view> Description() const override { return m_Description; }

    void Set(T Value) { m_Value = Value; }

    std::expected<void, std::string> TrySet(const std::string &Value) override {
        auto result = m_Validator(Value);
        if (result.has_value()) {
            return {};
        }
        return std::unexpected(result.error());
    }

    void Reset() override { m_Value = m_DefaultValue; }
};

#endif // CONFIG_VARIABLE_H
