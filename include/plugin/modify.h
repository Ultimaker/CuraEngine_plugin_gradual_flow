#ifndef PLUGIN_MODIFY_H
#define PLUGIN_MODIFY_H

#include "gradual_flow/gcode_path.h"
#include "plugin/broadcast.h"
#include "plugin/metadata.h"
#include "plugin/settings.h"

#include <boost/asio/awaitable.hpp>
#include <range/v3/view/drop.hpp>
#include <spdlog/spdlog.h>

#if __has_include(<coroutine>)
#include <coroutine>
#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>
#define USE_EXPERIMENTAL_COROUTINE
#endif
#include <filesystem>
#include <fstream>
#include <memory>

namespace plugin::gradual_flow
{
static int svg_counter = 0;

template<class T, class Rsp, class Req>
struct Generate
{
    using service_t = std::shared_ptr<T>;
    service_t generate_service{ std::make_shared<T>() };
    Broadcast::shared_settings_t settings{ std::make_shared<Broadcast::settings_t>() };
    std::shared_ptr<Metadata> metadata{ std::make_shared<Metadata>() };

    boost::asio::awaitable<void> run()
    {
        // previous_flow stores the previous flow value, the flow can differ per extruder
        // so each extruder is stored separately. The key in the map is the client-uuid
        // and the extruder_nr pair.
        std::unordered_map<std::string, std::unordered_map<int64_t, double>> previous_flow;
        while (true)
        {
            grpc::ServerContext server_context;

            cura::plugins::slots::gcode_paths::v0::modify::CallRequest request;
            grpc::ServerAsyncResponseWriter<Rsp> writer{ &server_context };
            co_await agrpc::request(&T::RequestCall, *generate_service, server_context, request, writer, boost::asio::use_awaitable);

            Rsp response;
            auto client_metadata = getUuid(server_context);
            const auto& extruder_nr = request.extruder_nr();

            grpc::Status status = grpc::Status::OK;
            try
            {
                auto extruder_settings = settings.get()->at(client_metadata);

                if (! extruder_settings.gradual_flow_enabled[extruder_nr])
                {
                    // If gradual flow is disabled, just return the original gcode paths
                    response.mutable_gcode_paths()->CopyFrom(request.gcode_paths());
                }
                else
                {
                    response.mutable_gcode_paths()->CopyFrom(request.gcode_paths());
//                    if (! previous_flow.contains(client_metadata))
//                    {
//                        previous_flow.emplace(client_metadata, std::unordered_map<int64_t, double>());
//                    }
//
//                    // Parse the gcode paths from the request
//                    std::vector<GCodePath> gcode_paths;
//
//                    for (int i = 0; i < request.gcode_paths().size(); ++i)
//                    {
//                        const auto& gcode_path_msg = request.gcode_paths().at(i);
//
//                        geometry::polyline<> points;
//                        /*
//                         * We need to add the last point of the previous path to the current path
//                         * since the paths in Cura are a connected line string and a new path begins
//                         * where the previous path ends (see figure below).
//                         *    {                Path A            } {          Path B        } { ...etc
//                         *    a.1-----------a.2------a.3---------a.4------b.1--------b.2--- c.1-------
//                         * For our purposes it is easier that each path is a separate line string, and
//                         * no knowledge of the previous path is needed.
//                         */
//                        if (i >= 1)
//                        {
//                            const auto& prev_path = &request.gcode_paths().at(i - 1).path().path();
//                            if (! prev_path->empty())
//                            {
//                                const auto& point = prev_path->at(prev_path->size() - 1); // prev_path->end();
//                                points.push_back(ClipperLib::IntPoint(point.x(), point.y()));
//                            }
//                        }
//
//                        for (const auto& point : gcode_path_msg.path().path())
//                        {
//                            points.push_back(ClipperLib::IntPoint(point.x(), point.y()));
//                        }
//
//                        GCodePath gcode_path{
//                            .original_gcode_path_data = &gcode_path_msg,
//                            .points = points,
//                        };
//
//                        gcode_paths.push_back(gcode_path);
//                    }
//
//                    constexpr auto non_zero_flow_view
//                            = ranges::views::transform([](const auto& path) { return path.flow(); })
//                            | ranges::views::filter([](const auto flow) { return flow > 0.0; });
//                    auto gcode_paths_non_zero_flow_view = gcode_paths | non_zero_flow_view;
//
//                    GCodeState state{
//                        .current_flow = previous_flow.at(client_metadata).contains(extruder_nr) ? previous_flow.at(client_metadata).at(extruder_nr) : 0.0,
//                        .flow_acceleration = request.layer_nr() == 0 ? extruder_settings.layer_0_max_flow_acceleration[extruder_nr] : extruder_settings.max_flow_acceleration[extruder_nr],
//                        .discretized_duration = extruder_settings.gradual_flow_discretisation_step_size[extruder_nr],
//                         // take the first path's target flow as the target flow, this might
//                         // not be correct, but it is safe to assume the target flow for the
//                         // next layer is the same as the target flow of the current layer
//                        .target_end_flow = gcode_paths_non_zero_flow_view.empty() ? 0.0 : ranges::front(gcode_paths_non_zero_flow_view),
//                    };
//
//                    const auto limited_flow_acceleration_paths = state.processGcodePaths(gcode_paths);
//                    auto limited_flow_acceleration_paths_non_zero_flow_view = limited_flow_acceleration_paths | non_zero_flow_view;
//                    previous_flow.at(client_metadata).emplace(extruder_nr, limited_flow_acceleration_paths_non_zero_flow_view.empty() ? 0.0 : ranges::back(limited_flow_acceleration_paths_non_zero_flow_view));
//                    // Copy newly generated paths to response
//
//                    for (const auto& [index, gcode_path] : limited_flow_acceleration_paths | ranges::views::enumerate)
//                    {
//                        // since the first point is added from the previous path in the request-parsing,
//                        // we should remove it here again. Note that the first point is added for every path
//                        // except the first one, so we should only remove it if it is not the first path
//                        const auto include_first_point = index == 0;
//                        auto gcode_path_message = gcode_path.toGrpcMessage(include_first_point);
//                        response.add_gcode_paths()->CopyFrom(gcode_path_message);
//                    }
                }
            }
            catch (const std::exception& e)
            {
                spdlog::error("Error: {}", e.what());
                status = grpc::Status(grpc::StatusCode::INTERNAL, static_cast<std::string>(e.what()));
            }

            if (! status.ok())
            {
                co_await agrpc::finish_with_error(writer, status, boost::asio::use_awaitable);
                continue;
            }
            co_await agrpc::finish(writer, response, status, boost::asio::use_awaitable);
        }
    }
};

} // namespace plugin::gradual_flow

#endif // PLUGIN_MODIFY_H
