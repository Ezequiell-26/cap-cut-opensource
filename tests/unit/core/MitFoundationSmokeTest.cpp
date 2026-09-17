#include <gtest/gtest.h>

#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <taskflow/taskflow.hpp>
#include <httplib.h>

enum class SmokeMode { Fast, Quality };

TEST(MitFoundationSmokeTest, JsonAndFormattingWork) {
    const nlohmann::json document{{"name", "CCOS"}, {"version", 7}};
    EXPECT_EQ(document.at("version").get<int>(), 7);
    EXPECT_EQ(fmt::format("{}:{}", document.at("name").get<std::string>(), 7), "CCOS:7");
    EXPECT_EQ(std::string(magic_enum::enum_name(SmokeMode::Quality)), "Quality");
}

TEST(MitFoundationSmokeTest, TaskflowExecutesDependencies) {
    taskflow::Taskflow taskflow;
    int value = 0;
    auto first = taskflow.emplace([&] { value = 21; });
    auto second = taskflow.emplace([&] { value *= 2; });
    first.precede(second);

    taskflow::Executor executor(1);
    executor.run(taskflow).wait();
    EXPECT_EQ(value, 42);
}

TEST(MitFoundationSmokeTest, LoggingAndHttpTypesCompile) {
    auto logger = spdlog::default_logger();
    ASSERT_NE(logger, nullptr);

    httplib::Client client("http://127.0.0.1:1");
    client.set_connection_timeout(1, 0);
    EXPECT_EQ(client.host(), "127.0.0.1");
}
