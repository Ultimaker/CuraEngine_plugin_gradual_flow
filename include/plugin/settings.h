#ifndef PLUGIN_SETTINGS_H
#define PLUGIN_SETTINGS_H

#include "cura/plugins/slots/broadcast/v0/broadcast.grpc.pb.h"

#include <semver.hpp>

#include <algorithm>
#include <cctype>
#include <ctre.hpp>
#include <locale>
#include <optional>
#include <string>
#include <unordered_map>

namespace plugin
{

struct Settings
{

    explicit Settings(const cura::plugins::slots::broadcast::v0::BroadcastServiceSettingsRequest& msg)
    {
    }

    static constexpr std::string_view getPattern(std::string_view pattern, std::string_view name)
    {
        if (auto [_, setting_namespace, plugin_name, plugin_version, pattern_name] = ctre::match<"^(.*?)::(.*?)@(.*?)::(.*?)$">(pattern);
            setting_namespace == "PLUGIN" && plugin_name == name)
        {
            return pattern_name;
        }
        return pattern;
    }
    
    static std::string settingKey(std::string_view short_key, std::string_view name, std::string_view version)
    {
        std::string lower_name{ name };
        auto semantic_version = semver::from_string(version);
        std::transform(
            lower_name.begin(),
            lower_name.end(),
            lower_name.begin(),
            [](const auto& c)
            {
                return std::tolower(c);
            });
        return fmt::format("_plugin__{}__{}_{}_{}__{}", lower_name, semantic_version.major, semantic_version.minor, semantic_version.patch, short_key);
    }
};

using settings_t = std::unordered_map<std::string, Settings>;

} // namespace plugin

#endif // PLUGIN_SETTINGS_H
