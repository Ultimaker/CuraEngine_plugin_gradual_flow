#ifndef PLUGIN_SETTINGS_H
#define PLUGIN_SETTINGS_H

#include "cura/plugins/slots/broadcast/v0/broadcast.grpc.pb.h"
#include "cura/plugins/slots/handshake/v0/handshake.grpc.pb.h"
#include "plugin/metadata.h"

#include <algorithm>
#include <cctype>
#include <ctre.hpp>
#include <locale>
#include <optional>
#include <semver.hpp>
#include <string>
#include <unordered_map>

namespace plugin
{

struct Settings
{
    std::shared_ptr<Metadata> metadata;
    bool gradual_flow_enabled = { false };
    double max_flow_acceleration = { 0.0 };
    double layer_0_max_flow_acceleration = { 0.0 };
    double gradual_flow_discretisation_step_size = { 0.0 };

    explicit Settings(const cura::plugins::slots::broadcast::v0::BroadcastServiceSettingsRequest& request, const std::shared_ptr<Metadata>& metadata)
        : metadata{ metadata }
    {
        const auto gradual_flow_enabled_setting = retrieveSettings("gradual_flow_enabled", request, metadata);
        const auto max_flow_acceleration_setting = retrieveSettings("max_flow_acceleration", request, metadata);
        const auto layer_0_max_flow_acceleration_setting = retrieveSettings("layer_0_max_flow_acceleration", request, metadata);
        const auto gradual_flow_discretisation_step_size_setting = retrieveSettings("gradual_flow_discretisation_step_size", request, metadata);

        if (! gradual_flow_enabled_setting.has_value() || ! max_flow_acceleration_setting.has_value() || ! layer_0_max_flow_acceleration_setting.has_value()
            || ! gradual_flow_discretisation_step_size_setting.has_value())
        {
            spdlog::error(
                "gradual_flow_enabled: {}, max_flow_acceleration: {}, layer_0_max_flow_acceleration: {}, gradual_flow_discretisation_step_size: {}",
                gradual_flow_enabled_setting.has_value(),
                max_flow_acceleration_setting.has_value(),
                layer_0_max_flow_acceleration_setting.has_value(),
                gradual_flow_discretisation_step_size_setting.has_value());
            throw std::runtime_error(fmt::format(
                "gradual_flow_enabled: {}, max_flow_acceleration: {}, layer_0_max_flow_acceleration: {}, gradual_flow_discretisation_step_size: {}",
                gradual_flow_enabled_setting.has_value(),
                max_flow_acceleration_setting.has_value(),
                layer_0_max_flow_acceleration_setting.has_value(),
                gradual_flow_discretisation_step_size_setting.has_value()));
        }
        gradual_flow_enabled = gradual_flow_enabled_setting.value() == "True" || gradual_flow_enabled_setting.value() == "true";

        max_flow_acceleration = std::stod(max_flow_acceleration_setting.value()) * 1e9;
        layer_0_max_flow_acceleration = std::stod(layer_0_max_flow_acceleration_setting.value()) * 1e9;
        gradual_flow_discretisation_step_size = std::stod(gradual_flow_discretisation_step_size_setting.value());
    }

    static std::optional<std::string>
        retrieveSettings(std::string settings_key, const cura::plugins::slots::broadcast::v0::BroadcastServiceSettingsRequest& request, const auto& metadata)
    {
        auto settings_key_ = settingKey(settings_key, metadata->plugin_name, metadata->plugin_version);
        if (request.global_settings().settings().contains(settings_key_))
        {
            return request.global_settings().settings().at(settings_key_);
        }

        return std::nullopt;
    }

    static bool validatePlugin(const cura::plugins::slots::handshake::v0::CallRequest& request, const std::shared_ptr<Metadata>& metadata)
    {
        return request.plugin_name() == metadata->plugin_name && request.plugin_version() == metadata->plugin_version;
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
