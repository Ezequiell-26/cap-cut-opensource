#include <gtest/gtest.h>

#include <fmt/format.h>
#include <glm/mat4x4.hpp>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <taskflow/taskflow.hpp>
#include <toml++/toml.hpp>
#include <httplib.h>

#include <string>

enum class SmokeMode { Fast, Quality };

TEST(MitFoundationSmokeTest, JsonFormattingAndTomlWork) {
    const nlohmann::json document{{"name", "CCOS"}, {"version", 7}};
    EXPECT_EQ(document.at("version").get<int>(), 7);
    EXPECT_EQ(fmt::format("{}:{}", document.at("name").get<std::string>(), 7), "CCOS:7");
    EXPECT_EQ(std::string(magic_enum::enum_name(SmokeMode::Quality)), "Quality");

    const auto config = toml::parse("width = 1920\nheight = 1080\n");
    ASSERT_TRUE(config["width"].value<int>().has_value());
    EXPECT_EQ(*config["width"].value<int>(), 1920);
}

TEST(MitFoundationSmokeTest, TaskflowAndGlmWork) {
    taskflow::Taskflow taskflow;
    int value = 0;
    const auto first = taskflow.emplace([&] { value = 21; });
    const auto second = taskflow.emplace([&] { value *= 2; });
    first.precede(second);

    taskflow::Executor executor(1);
    executor.run(taskflow).wait();
    EXPECT_EQ(value, 42);

    const glm::mat4 identity(1.0F);
    EXPECT_FLOAT_EQ(identity[0][0], 1.0F);
    EXPECT_FLOAT_EQ(identity[3][3], 1.0F);
}

TEST(MitFoundationSmokeTest, LoggingAndHttpTypesCompile) {
    auto logger = spdlog::default_logger();
    ASSERT_NE(logger, nullptr);

    httplib::Client client("http://127.0.0.1:1");
    client.set_connection_timeout(1, 0);
    SUCCEED();
}
