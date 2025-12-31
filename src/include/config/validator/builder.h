#ifndef CONFIG_BUILDER_H
#define CONFIG_BUILDER_H

#include "pipeline.h"

#include <algorithm>
#include <ranges>

template<typename T>
class ValidatorBuilder {
    ValidatorPipeline<T> m_Pipeline;

public:
    operator std::function<std::expected<T, std::string>(std::string)>() {
        return m_Pipeline;
    }

    // String Validators

    ValidatorBuilder &Trim() {
        m_Pipeline.AddStringValidator([](const std::string &value) -> std::expected<std::string, std::string> {
            std::string copy = value;
            auto not_space = [](const unsigned char c) { return !std::isspace(c); };
            copy.erase(
                std::ranges::find_if(copy | std::views::reverse, not_space).base(),
                copy.end());
            copy.erase(
                copy.begin(),
                std::ranges::find_if(copy, not_space));
            return copy;
        });
        return *this;
    }

    ValidatorBuilder &NotEmpty() {
        m_Pipeline.AddStringValidator([](std::string value) -> std::expected<std::string, std::string> {
            if (value.empty())
                return std::unexpected("Value should not be empty");
            return value;
        });
        return *this;
    }

    // Parsers

    ValidatorBuilder &Integer() {
        m_Pipeline.SetParser([](const std::string &value) -> std::expected<T, std::string> {
            if (value.empty())
                return std::unexpected("String should not be empty");

            try {
                if constexpr (std::is_same_v<T, int>) {
                    return std::stoi(value);
                } else if constexpr (std::is_same_v<T, long>) {
                    return std::stol(value);
                } else if constexpr (std::is_same_v<T, long long>) {
                    return std::stoll(value);
                }
            } catch (...) {
                return std::unexpected("Failed to parse integer");
            }
            return std::unexpected("Unsupported integer type");
        });
        return *this;
    }

    ValidatorBuilder &Float() {
        m_Pipeline.SetParser([](const std::string &value) -> std::expected<T, std::string> {
            if (value.empty())
                return std::unexpected("String should not be empty");

            try {
                if constexpr (std::is_same_v<T, float>) {
                    return std::stof(value);
                } else if constexpr (std::is_same_v<T, double>) {
                    return std::stod(value);
                }
            } catch (...) {
                return std::unexpected("Failed to parse float");
            }
            return std::unexpected("Unsupported float type");
        });
        return *this;
    }


    ValidatorBuilder &Boolean() {
        m_Pipeline.SetParser([](const std::string &value) -> std::expected<T, std::string> {
            if (value.empty())
                return std::unexpected("String should not be empty");

            try {
                if (value == "1" || value == "true") {
                    return true;
                }
                if (value == "0" || value == "false") {
                    return false;
                }
            } catch (...) {
                return std::unexpected("Failed to parse bool");
            }
            return std::unexpected("Unsupported bool value (1/true/0/false)");
        });
        return *this;
    }

    // Typed Validator

    ValidatorBuilder &Min(T minValue) {
        m_Pipeline.AddTypedValidator([minValue](T value) -> std::expected<T, std::string> {
            if (value < minValue)
                return std::unexpected("Value should be >=" + std::to_string(minValue));
            return value;
        });
        return *this;
    }

    ValidatorBuilder &Max(T maxValue) {
        m_Pipeline.AddTypedValidator([maxValue](T value) -> std::expected<T, std::string> {
            if (value > maxValue)
                return std::unexpected("Value should be <=" + std::to_string(maxValue));
            return value;
        });
        return *this;
    }

    ValidatorBuilder &Range(T minValue, T maxValue) {
        m_Pipeline.AddTypedValidator([minValue, maxValue](T value) -> std::expected<T, std::string> {
            if (value < minValue || value > maxValue)
                return std::unexpected("Value should be >=" + std::to_string(minValue) +
                                       " and <=" + std::to_string(maxValue));
            return value;
        });
        return *this;
    }

    // Custom

    ValidatorBuilder &Custom(ValidatorPipeline<T>::StringValidator validator) {
        m_Pipeline.AddStringValidator(std::move(validator));
        return *this;
    }

    ValidatorBuilder &CustomTyped(ValidatorPipeline<T>::TypedValidator validator) {
        m_Pipeline.AddTypedValidator(std::move(validator));
        return *this;
    }
};

template<typename T>
ValidatorBuilder<T> Validator() {
    return ValidatorBuilder<T>();
}

// Shortcuts
namespace Validators {
    inline ValidatorBuilder<int> IntRanged(int Min, int Max) {
        return ValidatorBuilder<int>().Trim().NotEmpty().Integer().Range(Min, Max);
    }

    inline ValidatorBuilder<float> FloatRanged(float Min, float Max) {
        return ValidatorBuilder<float>().Trim().NotEmpty().Float().Range(Min, Max);
    }

    inline ValidatorBuilder<std::string> StringNonEmpty() {
        return ValidatorBuilder<std::string>().Trim().NotEmpty();
    }

    inline ValidatorBuilder<bool> Boolean() {
        return ValidatorBuilder<bool>().Trim().NotEmpty().Boolean();
    }
}

#endif // CONFIG_BUILDER_H
