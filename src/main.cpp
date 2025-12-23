#include <iostream>

#include "lib/variable.h"
#include "lib/registry.h"
#include "lib/validator/builder.h"

int main() {
    CONFIG_STRING("veryImportantString", "fas", Validators::StringNonEmpty());
    CONFIG_INT("integer", 512, Validators::IntRanged(0, 500));
    CONFIG_FLOAT("getReal", 22.8, Validators::FloatRanged(0, 200));

    Config().LoadFromFile("config.json");

    for (const auto& var : Config().ListAll()) {
        auto info = *Config().GetInfo(var);
        std::cout << std::format("{}: {} = {}(def: {})", info.name, info.type, info.value, info.default_value) << std::endl;
    }

    Config().SaveToFile("config.json");
    Config().ExportTemplate("config_all.json");

    return 0;
}
