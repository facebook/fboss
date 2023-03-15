#include "RackmonConfig.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace rackmonsvc;
using nlohmann::json;

TEST(RackmonConfig, Interface) {
  json ifaceConfig = json::parse(getInterfaceConfig());
  ASSERT_TRUE(ifaceConfig.contains("interfaces"));
  ASSERT_EQ(ifaceConfig["interfaces"].size(), 3);
  EXPECT_EQ(ifaceConfig["interfaces"][0]["device_path"], "/dev/ttyUSB1");
  EXPECT_EQ(ifaceConfig["interfaces"][1]["device_path"], "/dev/ttyUSB2");
  EXPECT_EQ(ifaceConfig["interfaces"][2]["device_path"], "/dev/ttyUSB3");
}

TEST(RackmonConfig, RegisterMap) {
  const std::vector<std::string> mapsConfig = getRegisterMapConfig();
  ASSERT_EQ(mapsConfig.size(), 1);
  json maps = json::parse(mapsConfig.at(0));
  ASSERT_EQ(maps["name"], "ORV2_PSU");
  ASSERT_EQ(maps["probe_register"], 104);
  ASSERT_GE(maps["registers"].size(), 30);
  ASSERT_EQ(maps["registers"][0]["name"], "PSU MFR MODEL");
}
