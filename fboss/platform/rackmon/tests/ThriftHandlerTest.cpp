// Copyright 2021-present Facebook. All Rights Reserved.
#include <gtest/gtest.h>
#include "fboss/platform/rackmon/if/gen-cpp2/rackmonsvc_types.h"

// Forward declare public free functions from RackmonThriftHandler.cpp
namespace rackmonsvc {
ModbusDeviceType typeFromString(const std::string& str);
std::string typeToString(ModbusDeviceType type);
} // namespace rackmonsvc

TEST(ThriftHandlerTest, TypeFromString) {
  // Valid conversions
  EXPECT_EQ(
      rackmonsvc::typeFromString("ORV2_PSU"),
      rackmonsvc::ModbusDeviceType::ORV2_PSU);
  EXPECT_EQ(
      rackmonsvc::typeFromString("ORV3_PSU"),
      rackmonsvc::ModbusDeviceType::ORV3_PSU);
  EXPECT_EQ(
      rackmonsvc::typeFromString("ORV3_BBU"),
      rackmonsvc::ModbusDeviceType::ORV3_BBU);

  // Invalid inputs throw
  EXPECT_THROW(rackmonsvc::typeFromString("UNKNOWN_TYPE"), std::runtime_error);
  EXPECT_THROW(rackmonsvc::typeFromString(""), std::runtime_error);
}

TEST(ThriftHandlerTest, TypeToString) {
  // Valid conversions
  EXPECT_EQ(
      rackmonsvc::typeToString(rackmonsvc::ModbusDeviceType::ORV2_PSU),
      "ORV2_PSU");
  EXPECT_EQ(
      rackmonsvc::typeToString(rackmonsvc::ModbusDeviceType::ORV3_PSU),
      "ORV3_PSU");
  EXPECT_EQ(
      rackmonsvc::typeToString(rackmonsvc::ModbusDeviceType::ORV3_BBU),
      "ORV3_BBU");

  // Invalid input throws
  EXPECT_THROW(
      rackmonsvc::typeToString(static_cast<rackmonsvc::ModbusDeviceType>(999)),
      std::runtime_error);
}
