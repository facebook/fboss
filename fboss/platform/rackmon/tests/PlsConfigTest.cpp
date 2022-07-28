#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "RackmonConfig.h"

using namespace rackmonsvc;
using nlohmann::json;

void TestPlsLine(json& lineConfig) {
  ASSERT_TRUE(lineConfig.contains("gpioChip"));
  ASSERT_FALSE(lineConfig["gpioChip"].empty());
  ASSERT_TRUE(lineConfig.contains("offset"));
  ASSERT_TRUE(lineConfig["offset"] >= 0);
  ASSERT_TRUE(lineConfig.contains("type"));
  ASSERT_TRUE(
      lineConfig["type"] == "power" || lineConfig["type"] == "redundancy");
}

void TestPlsPort(json& portConfig) {
  ASSERT_TRUE(portConfig.contains("name"));
  ASSERT_TRUE(portConfig.contains("lines"));
  EXPECT_EQ(portConfig["lines"].size(), 2);
  for (auto& gpioLine : portConfig["lines"]) {
    TestPlsLine(gpioLine);
  }
}

TEST(RackmonPlsConfig, PlsConfigTest) {
  json plsConfig = json::parse(getRackmonPlsConfig());

  ASSERT_TRUE(plsConfig.contains("ports"));
  ASSERT_EQ(plsConfig["ports"].size(), 3);
  for (auto& port : plsConfig["ports"]) {
    TestPlsPort(port);
  }
}
