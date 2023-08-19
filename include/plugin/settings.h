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
        bool gradual_flow_enabled = { false };
        double max_flow_acceleration = { 0.0 };
        double layer_0_max_flow_acceleration = { 0.0 };
        double gradual_flow_discretisation_step_size = { 0.0 };

        explicit Settings(const cura::plugins::slots::broadcast::v0::BroadcastServiceSettingsRequest& msg)
        {
            const auto global_settings = msg.global_settings().settings();

            gradual_flow_enabled = [global_settings](){
                const auto gradual_flow_enabled_str = global_settings.at(settingKey("gradual_flow_enabled", "gradualflowplugin", "0.1.0"));
                if (gradual_flow_enabled_str == "true" || gradual_flow_enabled_str == "True")
                {
                    return true;
                }
                if (gradual_flow_enabled_str == "false" || gradual_flow_enabled_str == "False")
                {
                    return false;
                }
                throw std::runtime_error("gradual_flow_enabled must be either True or False");
            }();

            max_flow_acceleration = std::atof(global_settings.at(settingKey("max_flow_acceleration", "gradualflowplugin", "0.1.0")).c_str()) * 1e9;
            layer_0_max_flow_acceleration = std::atof(global_settings.at(settingKey("layer_0_max_flow_acceleration", "gradualflowplugin", "0.1.0")).c_str()) * 1e9;
            gradual_flow_discretisation_step_size = std::atof(global_settings.at(settingKey("gradual_flow_discretisation_step_size", "gradualflowplugin", "0.1.0")).c_str());
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
