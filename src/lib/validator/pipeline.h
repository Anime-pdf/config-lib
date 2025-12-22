#ifndef CONFIG_PIPELINE_H
#define CONFIG_PIPELINE_H

#include <expected>
#include <functional>
#include <string>

template<typename T>
class ValidatorPipeline {
public:
    using StringValidator = std::function<std::expected<std::string, std::string>(std::string)>;
    using TypedValidator = std::function<std::expected<T, std::string>(T)>;

private:
    std::vector<StringValidator> m_StringValidators;
    std::function<std::expected<T, std::string>(std::string)> m_Parser;
    std::vector<TypedValidator> m_TypedValidators;

public:
    ValidatorPipeline &AddStringValidator(StringValidator validator) {
        m_StringValidators.push_back(std::move(validator));
        return *this;
    }

    ValidatorPipeline &SetParser(std::function<std::expected<T, std::string>(std::string)> parser) {
        m_Parser = std::move(parser);
        return *this;
    }

    ValidatorPipeline &AddTypedValidator(TypedValidator validator) {
        m_TypedValidators.push_back(std::move(validator));
        return *this;
    }

    std::expected<T, std::string> operator()(std::string value) const {
        for (const auto &validator: m_StringValidators) {
            auto result = validator(value);
            if (!result.has_value())
                return std::unexpected(result.error());
            value = result.value();
        }

        if (!m_Parser)
            return std::unexpected("No parser configured");

        auto parsed = m_Parser(value);
        if (!parsed.has_value())
            return parsed;

        T finalValue = parsed.value();
        for (const auto &validator: m_TypedValidators) {
            auto result = validator(finalValue);
            if (!result.has_value())
                return result;
            finalValue = result.value();
        }

        return finalValue;
    }
};

#endif // CONFIG_PIPELINE_H
