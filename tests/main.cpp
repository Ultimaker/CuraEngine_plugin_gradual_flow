#define CATCH_CONFIG_MAIN

#include "gradual_flow/gcode_path.h"

#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <range/v3/algorithm/any_of.hpp>
#include <range/v3/view/transform.hpp>

/*
 * Mocks a GCodePath message with a given velocity. The default configuration has a line width of 400, a layer thickness
 * of 200, and all flow/line width ratios set to 1.0. This means that the extrusion volume is 400 * 200 * 1.0 = 80000.
 *
 * @param velocity The velocity to set in the message. Defaults to 100.
 *
 * @return A GCodePath message with the given velocity.
 */
cura::plugins::v0::GCodePath mock_msg(double velocity = 100)
{
    cura::plugins::v0::GCodePath original_gcode_path_data;
    original_gcode_path_data.set_flow(1.0);
    original_gcode_path_data.set_width_factor(1.0);
    original_gcode_path_data.mutable_config()->set_line_width(400);
    original_gcode_path_data.mutable_config()->set_layer_thickness(200);
    original_gcode_path_data.mutable_config()->set_flow_ratio(1.0);
    original_gcode_path_data.mutable_config()->mutable_speed_derivatives()->set_velocity(velocity);
    return original_gcode_path_data;
}

TEST_CASE("segment duration long line")
{
    // Make sure all partition-segments will have the discretized
    // duration; given a long line as input.
    const auto original_gcode_path_data = mock_msg();
    const plugin::gradual_flow::GCodePath path {
        .original_gcode_path_data = &original_gcode_path_data,
        .points = { { 0, 0 }, { 0, 100000000 } },
    };

    const auto discretized_duration = .1;

    plugin::gradual_flow::GCodeState state {
        .current_flow = 0.,
        .flow_acceleration = 1000000000.,
        .discretized_duration = discretized_duration,
    };

    const auto limited_flow_acceleration_paths = state.processGcodePaths({ path });

    for (const auto& discretized_path: limited_flow_acceleration_paths | ranges::views::drop_last(1))
    {
        REQUIRE(discretized_path.totalDuration() == Catch::Approx(discretized_duration).epsilon(0.01));
    }
}

TEST_CASE("Total length remains same")
{
    // Make sure the total length of the discretized path is the
    // same as the original path; given a long line as input.
    const auto original_gcode_path_data = mock_msg();
    const plugin::gradual_flow::GCodePath path {
        .original_gcode_path_data = &original_gcode_path_data,
        .points = { { 0, 0 }, { 0, 100000000 } },
    };

    const auto discretized_duration = .1;

    plugin::gradual_flow::GCodeState state {
        .current_flow = 0.,
        .flow_acceleration = 1000000000.,
        .discretized_duration = discretized_duration,
    };

    const auto limited_flow_acceleration_paths = state.processGcodePaths({ path });

    auto total_length = .0;
    for (const auto& discretized_path: limited_flow_acceleration_paths)
    {
        total_length += discretized_path.totalLength();
    }
    REQUIRE(total_length == Catch::Approx(path.totalLength()).epsilon(0.01));
}

TEST_CASE("Total length remains same short line segments")
{
    // Make sure the total length of the discretized path is the same as the
    // original path; given a single path with short line segments as input.
    plugin::gradual_flow::geometry::polyline<> points;
    for (int i = 0; i < 100000000; i+= 100)
    {
        points.emplace_back(0, i);
    }

    const auto original_gcode_path_data = mock_msg();
    const plugin::gradual_flow::GCodePath path {
        .original_gcode_path_data = &original_gcode_path_data,
        .points = points,
    };

    const auto discretized_duration = .1;

    plugin::gradual_flow::GCodeState state {
        .current_flow = 0.,
        .flow_acceleration = 1000000000.,
        .discretized_duration = discretized_duration,
    };

    const auto limited_flow_acceleration_paths = state.processGcodePaths({ path });

    auto total_length = .0;
    for (const auto& discretized_path: limited_flow_acceleration_paths)
    {
        total_length += discretized_path.totalLength();
    }
    REQUIRE(total_length == Catch::Approx(path.totalLength()).epsilon(0.01));
}

TEST_CASE("segment duration small segments")
{
    // Make sure all partition-segments will have the discretized
    // duration; given a single path with short line segments as input.
    plugin::gradual_flow::geometry::polyline<> points;
    for (int i = 0; i < 100000000; i+= 100)
    {
        points.emplace_back(0, i);
    }

    const auto original_gcode_path_data = mock_msg();
    const plugin::gradual_flow::GCodePath path {
        .original_gcode_path_data = &original_gcode_path_data,
        .points = points,
    };

    const auto discretized_duration = .1;

    plugin::gradual_flow::GCodeState state
    {
        .current_flow = 0.,
        .flow_acceleration = 1000000000.,
        .discretized_duration = discretized_duration,
    };

    const auto limited_flow_acceleration_paths = state.processGcodePaths({ path });

    for (const auto& discretized_path: limited_flow_acceleration_paths | ranges::views::drop_last(1))
    {
        REQUIRE(discretized_path.totalDuration() == Catch::Approx(discretized_duration).epsilon(0.01));
    }
}

TEST_CASE("forward discretization steps")
{
    // Make sure a path is partitioned into the correct number of
    // segments; given a single path with short line segments as input.
    // With each discretization step the flow is increased by the
    // flow_acceleration * discretized_duration. The numer of steps
    // should thus be equal to the target flow divided by the flow increase
    // per step.
    const auto original_gcode_path_data = mock_msg();
    plugin::gradual_flow::geometry::polyline<> points;
    for (int i = 0; i < 100000000; i+= 100)
    {
        points.emplace_back(0, i);
    }

    plugin::gradual_flow::GCodePath path {
        .original_gcode_path_data = &original_gcode_path_data,
        .points = points,
    };

    const auto discretized_duration = 2.;
    const auto flow_acceleration = 100000000.;
    const auto initial_flow = 200000000.;

    plugin::gradual_flow::GCodeState state
    {
        .current_flow = initial_flow,
        .flow_acceleration = flow_acceleration,
        .discretized_duration = discretized_duration,
    };

    const auto limited_flow_acceleration_paths = state.processGcodePaths({ path });

    const auto flow_delta = path.flow() - initial_flow;
    REQUIRE(limited_flow_acceleration_paths.size() == ceil(flow_delta / (flow_acceleration * discretized_duration)));
}

TEST_CASE("discretization steps backward")
{
    // see forward discretization steps; but now the flow is decreased
    // instead of increased. This is achieved by initializing two
    const auto original_gcode_path_data_100mm_s = mock_msg(100); // 100mm/s
    const auto original_gcode_path_data_10mm_s = mock_msg(10); // 10mm/s
    plugin::gradual_flow::GCodePath path_fast
    {
        .original_gcode_path_data = &original_gcode_path_data_100mm_s,
        .points = { { 0, 0 }, { 0, 100000000 } },
    };
    plugin::gradual_flow::GCodePath path_slow
    {
        .original_gcode_path_data = &original_gcode_path_data_10mm_s,
        .points = { { 0, 100000000 }, { 0, 100000010 } },
    };
    std::vector<plugin::gradual_flow::GCodePath> paths { path_fast, path_slow };

    const auto discretized_duration = 2.;
    const auto flow_acceleration = 100000000.;

    plugin::gradual_flow::GCodeState state
    {
        .current_flow = path_fast.flow(),
        .flow_acceleration = flow_acceleration,
        .discretized_duration = discretized_duration,
    };

    const auto limited_flow_acceleration_paths = state.processGcodePaths(paths);

    // we need -1 here since the original path consisted of 2 paths
    const auto flow_delta = path_fast.flow() - path_slow.flow();
    REQUIRE(limited_flow_acceleration_paths.size() - 1 == ceil(flow_delta / (flow_acceleration * discretized_duration)));
}

TEST_CASE("flow limit forward backward target speed reached")
{
    // relatively long path with a high target speed in the middle with at
    // both ends a lower target speed. The flow should increase and decrease
    // since the flow acceleration
    const auto original_gcode_path_data_100mm_s = mock_msg(100); // 100mm/s
    const auto original_gcode_path_data_10mm_s = mock_msg(10); // 10mm/s
    plugin::gradual_flow::GCodePath path_left
    {
        .original_gcode_path_data = &original_gcode_path_data_10mm_s,
        .points = { { 0, 20000 }, { 10000, 10000 } },
    };
    plugin::gradual_flow::GCodePath path_middle
    {
        .original_gcode_path_data = &original_gcode_path_data_100mm_s,
        .points = { { 10000, 10000 }, { 190000, 10000 } },
    };
    plugin::gradual_flow::GCodePath path_right
    {
        .original_gcode_path_data = &original_gcode_path_data_10mm_s,
        .points = { { 190000, 10000 }, { 200000, 20000 } },
    };

    std::vector<plugin::gradual_flow::GCodePath> paths { path_left, path_middle, path_right };

    const auto discretized_duration = 0.01;
    const auto flow_acceleration = 10000000000.;

    plugin::gradual_flow::GCodeState state
    {
        .current_flow = paths.front().flow(),
        .flow_acceleration = flow_acceleration,
        .discretized_duration = discretized_duration,
    };

    const auto limited_flow_acceleration_paths = state.processGcodePaths(paths);

    const auto target_flow = Catch::Approx(path_middle.flow()).epsilon(0.01);
    REQUIRE(ranges::any_of(limited_flow_acceleration_paths, [target_flow](auto path) { return path.flow() == target_flow; }));
}

TEST_CASE("flow limit forward backward target speed not reached")
{
    // relatively long path with a high target speed in the middle with at
    // both ends a lower target speed. The flow should increase and decrease
    // since the flow acceleration is set just high enough (10mm^3/s^2) the target.
    // speed is reached in the middle
    const auto original_gcode_path_data_100mm_s = mock_msg(100); // 100mm/s
    const auto original_gcode_path_data_10mm_s = mock_msg(10); // 10mm/s
    plugin::gradual_flow::GCodePath path_left
    {
            .original_gcode_path_data = &original_gcode_path_data_10mm_s,
            .points = { { 0, 20000 }, { 10000, 10000 } },
    };
    plugin::gradual_flow::GCodePath path_middle
    {
            .original_gcode_path_data = &original_gcode_path_data_100mm_s,
            .points = { { 10000, 10000 }, { 190000, 10000 } },
    };
    plugin::gradual_flow::GCodePath path_right
    {
            .original_gcode_path_data = &original_gcode_path_data_10mm_s,
            .points = { { 190000, 10000 }, { 200000, 20000 } },
    };

    std::vector<plugin::gradual_flow::GCodePath> paths { path_left, path_middle, path_right };

    const auto discretized_duration = 0.1;
    const auto flow_acceleration = 3000000000.;

    plugin::gradual_flow::GCodeState state
    {
        .current_flow = paths.front().flow(),
        .flow_acceleration = flow_acceleration,
        .discretized_duration = discretized_duration,
    };

    const auto limited_flow_acceleration_paths = state.processGcodePaths(paths);

    for (auto& path: limited_flow_acceleration_paths) {
        REQUIRE(path.flow() < path_middle.flow());
    }
}
