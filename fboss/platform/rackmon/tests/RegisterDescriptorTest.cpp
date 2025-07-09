// Copyright 2021-present Facebook. All Rights Reserved.
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "Register.h"

using namespace std;
using namespace testing;
using namespace rackmon;

//--------------------------------------------------------
// RegisterDescriptorTests
//--------------------------------------------------------

TEST(RegisterDescriptorTest, JSONConversionDefaults) {
  nlohmann::json desc = nlohmann::json::parse(R"({
    "begin": 0,
    "length": 8,
    "name": "MFG_MODEL"
  })");
  RegisterDescriptor d = desc;
  EXPECT_EQ(d.begin, 0);
  EXPECT_EQ(d.length, 8);
  EXPECT_EQ(d.name, "MFG_MODEL");
  EXPECT_EQ(d.keep, 1);
  EXPECT_EQ(d.storeChangesOnly, false);
  EXPECT_EQ(d.format, RegisterValueType::HEX);
}

TEST(RegisterDescriptorTest, JSONConversionEnforceMandatory) {
  RegisterDescriptor d;
  nlohmann::json desc1 = nlohmann::json::parse(R"({
    "length": 8,
    "name": "MFG_MODEL"
  })");
  EXPECT_THROW(d = desc1, nlohmann::json::out_of_range);

  nlohmann::json desc2 = nlohmann::json::parse(R"({
    "begin": 8,
    "name": "MFG_MODEL"
  })");
  EXPECT_THROW(d = desc2, nlohmann::json::out_of_range);

  nlohmann::json desc3 = nlohmann::json::parse(R"({
    "begin": 0,
    "length": 8
  })");
  EXPECT_THROW(d = desc2, nlohmann::json::out_of_range);
}

TEST(RegisterDescriptorTest, JSONConversionString) {
  // TODO Change JSON
  nlohmann::json desc = nlohmann::json::parse(R"({
    "begin": 0,
    "length": 8,
    "format": "STRING",
    "name": "MFG_MODEL"
  })");
  RegisterDescriptor d = desc;
  EXPECT_EQ(d.begin, 0);
  EXPECT_EQ(d.length, 8);
  EXPECT_EQ(d.name, "MFG_MODEL");
  EXPECT_EQ(d.keep, 1);
  EXPECT_EQ(d.storeChangesOnly, false);
  EXPECT_EQ(d.format, RegisterValueType::STRING);
}

TEST(RegisterDescriptorTest, JSONConversionDecimalKeep) {
  nlohmann::json desc = nlohmann::json::parse(R"({
    "begin": 156,
    "length": 1,
    "format": "INTEGER",
    "keep": 10,
    "name": "Set fan speed"
  })");
  RegisterDescriptor d = desc;
  EXPECT_EQ(d.begin, 156);
  EXPECT_EQ(d.length, 1);
  EXPECT_EQ(d.name, "Set fan speed");
  EXPECT_EQ(d.keep, 10);
  EXPECT_EQ(d.storeChangesOnly, false);
  EXPECT_EQ(d.format, RegisterValueType::INTEGER);
}

TEST(RegisterDescriptorTest, JSONConversionFixed) {
  nlohmann::json desc = nlohmann::json::parse(R"({
    "begin": 127,
    "length": 1,
    "format": "FLOAT",
    "precision": 6,
    "name": "Input VAC"
  })");
  RegisterDescriptor d = desc;
  EXPECT_EQ(d.begin, 127);
  EXPECT_EQ(d.length, 1);
  EXPECT_EQ(d.name, "Input VAC");
  EXPECT_EQ(d.keep, 1);
  EXPECT_EQ(d.storeChangesOnly, false);
  EXPECT_EQ(d.format, RegisterValueType::FLOAT);
  EXPECT_EQ(d.precision, 6);
  EXPECT_NEAR(d.scale, 1.0, 0.1);
  EXPECT_NEAR(d.shift, 0.0, 0.1);
}

TEST(RegisterDescriptorTest, JSONConversionFixedScale) {
  nlohmann::json desc = nlohmann::json::parse(R"({
    "begin": 127,
    "length": 1,
    "format": "FLOAT",
    "precision": 6,
    "scale": 0.1,
    "shift": 4.2,
    "name": "Input VAC"
  })");
  RegisterDescriptor d = desc;
  EXPECT_EQ(d.begin, 127);
  EXPECT_EQ(d.length, 1);
  EXPECT_EQ(d.name, "Input VAC");
  EXPECT_EQ(d.keep, 1);
  EXPECT_EQ(d.storeChangesOnly, false);
  EXPECT_EQ(d.format, RegisterValueType::FLOAT);
  EXPECT_EQ(d.precision, 6);
  EXPECT_NEAR(d.scale, 0.1, 0.01);
  EXPECT_NEAR(d.shift, 4.2, 0.01);
}

TEST(RegisterDescriptorTest, JSONConversionFixedMissingPrec) {
  nlohmann::json desc = nlohmann::json::parse(R"({
    "begin": 127,
    "length": 1,
    "format": "FLOAT",
    "name": "Input VAC"
  })");
  RegisterDescriptor d;
  EXPECT_THROW(d = desc, nlohmann::json::out_of_range);
}

TEST(RegisterDescriptorTest, JSONConversionTableChangesOnly) {
  nlohmann::json desc = nlohmann::json::parse(R"({
    "begin": 105,
    "length": 1,
    "keep": 10,
    "changes_only": true,
    "format": "FLAGS",
    "name": "Battery Status register",
    "flags": [
      [2, "End of Life"],
      [1, "Low Voltage  BBU Voltage <= 33.8"],
      [0, "BBU Fail"]
    ]
  })");
  RegisterDescriptor d = desc;
  EXPECT_EQ(d.begin, 105);
  EXPECT_EQ(d.length, 1);
  EXPECT_EQ(d.keep, 10);
  EXPECT_EQ(d.storeChangesOnly, true);
  EXPECT_EQ(d.name, "Battery Status register");
  EXPECT_EQ(d.format, RegisterValueType::FLAGS);
  std::vector<std::tuple<uint8_t, std::string>> expected = {
      {2, "End of Life"},
      {1, "Low Voltage  BBU Voltage <= 33.8"},
      {0, "BBU Fail"}};
  EXPECT_EQ(d.flags, expected);
}

TEST(RegisterDescriptorTest, JSONConversionTableMissing) {
  nlohmann::json desc = nlohmann::json::parse(R"({
    "begin": 105,
    "length": 1,
    "keep": 10,
    "changes_only": true,
    "format": "FLAGS",
    "name": "Battery Status register"
  })");
  RegisterDescriptor d;
  EXPECT_THROW(d = desc, nlohmann::json::out_of_range);
}
