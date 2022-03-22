// Copyright 2021-present Facebook. All Rights Reserved.
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "Register.h"

using namespace std;
using namespace testing;
using namespace rackmon;

TEST(RegisterValueTest, Hex) {
  RegisterValue val({1, 2});
  std::vector<uint8_t> exp{0x00, 0x01, 0x00, 0x02};
  EXPECT_EQ(val.type, RegisterValueType::HEX);
  EXPECT_EQ(val.value.hexValue, exp);

  nlohmann::json j = val;
  EXPECT_TRUE(j.is_object());
  EXPECT_TRUE(j.contains("type") && j["type"].is_string());
  EXPECT_TRUE(j.contains("time") && j["time"].is_number_integer());
  EXPECT_TRUE(j.contains("value") && j["value"].is_array());
  EXPECT_EQ(std::string(j["type"]), "hex");
  EXPECT_EQ(j["time"], 0);
  EXPECT_EQ(j["value"].size(), 4);
  EXPECT_EQ(std::vector<uint8_t>(j["value"]), exp);
}

TEST(RegisterValueTest, STRING) {
  RegisterDescriptor d;
  d.format = RegisterValueType::STRING;
  RegisterValue val(
      {0x3730, 0x302d, 0x3031, 0x3436, 0x3731, 0x2d30, 0x3030, 0x3020},
      d,
      0x12345678);
  EXPECT_EQ(val.type, RegisterValueType::STRING);
  EXPECT_EQ(val.value.strValue, "700-014671-0000 ");
  EXPECT_EQ(val.timestamp, 0x12345678);

  nlohmann::json j = val;
  EXPECT_TRUE(j.is_object());
  EXPECT_TRUE(j.contains("type") && j["type"].is_string());
  EXPECT_TRUE(j.contains("time") && j["time"].is_number_integer());
  EXPECT_TRUE(j.contains("value") && j["value"].is_string());
  EXPECT_EQ(std::string(j["type"]), "string");
  EXPECT_EQ(j["time"], 0x12345678);
  EXPECT_EQ(j["value"], "700-014671-0000 ");
}

TEST(RegisterValueTest, INTEGER) {
  RegisterDescriptor d;
  d.format = RegisterValueType::INTEGER;
  RegisterValue val({0x1234, 0x5678}, d, 0x12345678);
  EXPECT_EQ(val.type, RegisterValueType::INTEGER);
  EXPECT_EQ(val.value.intValue, 0x12345678);

  nlohmann::json j = val;
  EXPECT_TRUE(j.is_object());
  EXPECT_TRUE(j.contains("type") && j["type"].is_string());
  EXPECT_TRUE(j.contains("time") && j["time"].is_number_integer());
  EXPECT_TRUE(j.contains("value") && j["value"].is_number_integer());
  EXPECT_EQ(std::string(j["type"]), "integer");
  EXPECT_EQ(j["time"], 0x12345678);
  EXPECT_TRUE(j["value"].is_number_integer());
  EXPECT_EQ(j["value"], 0x12345678);
}

TEST(RegisterValueTest, LITTLE_INTEGER) {
  RegisterDescriptor d;
  d.format = RegisterValueType::INTEGER;
  d.endian = RegisterEndian::LITTLE;
  RegisterValue val({0x1234, 0x5678}, d, 0x12345678);
  EXPECT_EQ(val.type, RegisterValueType::INTEGER);
  EXPECT_EQ(val.value.intValue, 0x78563412);
}

TEST(RegisterValueTest, FLOAT) {
  RegisterDescriptor d;
  d.format = RegisterValueType::FLOAT;
  d.precision = 11;
  RegisterValue val({0x64fc}, d, 0x12345678);
  EXPECT_EQ(val.type, RegisterValueType::FLOAT);
  EXPECT_NEAR(val.value.floatValue, 12.623, 0.001);

  nlohmann::json j = val;
  EXPECT_TRUE(j.is_object());
  EXPECT_TRUE(j.contains("type") && j["type"].is_string());
  EXPECT_TRUE(j.contains("time") && j["time"].is_number_integer());
  EXPECT_TRUE(j.contains("value") && j["value"].is_number_float());
  EXPECT_EQ(std::string(j["type"]), "float");
  EXPECT_EQ(j["time"], 0x12345678);
  EXPECT_NEAR(j["value"], 12.623, 0.001);
}

TEST(RegisterValueTest, FLAGS) {
  RegisterDescriptor d;
  d.format = RegisterValueType::FLAGS;
  d.flags = {{0, "HELLO"}, {1, "WORLD"}};
  RegisterValue val1({0x0003}, d, 0x12345678);
  EXPECT_EQ(val1.type, RegisterValueType::FLAGS);
  std::string expstr1 = "\n*[1] <0> HELLO\n*[1] <1> WORLD";
  RegisterValue::FlagsType exp1 = {{true, "HELLO", 0}, {true, "WORLD", 1}};
  EXPECT_EQ(val1.value.flagsValue, exp1);

  RegisterValue val2({0x0000}, d, 0x12345678);
  EXPECT_EQ(val1.type, RegisterValueType::FLAGS);
  RegisterValue::FlagsType exp2 = {{false, "HELLO", 0}, {false, "WORLD", 1}};
  EXPECT_EQ(val2.value.flagsValue, exp2);

  RegisterValue val3({0x0002}, d, 0x12345678);
  EXPECT_EQ(val3.type, RegisterValueType::FLAGS);
  RegisterValue::FlagsType exp3 = {{false, "HELLO", 0}, {true, "WORLD", 1}};
  EXPECT_EQ(val3.value.flagsValue, exp3);

  nlohmann::json j = val3;
  EXPECT_TRUE(j.is_object());
  EXPECT_TRUE(j.contains("type") && j["type"].is_string());
  EXPECT_TRUE(j.contains("time") && j["time"].is_number_integer());
  EXPECT_TRUE(j.contains("value") && j["value"].is_array());
  EXPECT_EQ(std::string(j["type"]), "flags");
  EXPECT_EQ(j["time"], 0x12345678);
  EXPECT_EQ(j["value"].size(), 2);
  EXPECT_TRUE(j["value"][0].is_array());
  EXPECT_TRUE(j["value"][1].is_array());
  EXPECT_EQ(j["value"][0].size(), 3);
  EXPECT_EQ(j["value"][1].size(), 3);
  EXPECT_TRUE(j["value"][0][0].is_boolean());
  EXPECT_TRUE(j["value"][1][0].is_boolean());
  EXPECT_TRUE(j["value"][0][1].is_string());
  EXPECT_TRUE(j["value"][1][1].is_string());
  EXPECT_FALSE(j["value"][0][0]);
  EXPECT_TRUE(j["value"][1][0]);
  EXPECT_EQ(std::string(j["value"][0][1]), "HELLO");
  EXPECT_EQ(std::string(j["value"][1][1]), "WORLD");
}

TEST(RegisterValueTest, LargeFlags) {
  RegisterDescriptor d;
  d.format = RegisterValueType::FLAGS;
  d.flags = {
      {0, "HELLO"},
      {
          31,
          "WORLD",
      },
      {32, "HELLO2"},
      {63, "WORLD2"}};
  RegisterValue val({0x8000, 0x0000, 0x0000, 0x0001}, d, 0x12345678);
  EXPECT_EQ(val.type, RegisterValueType::FLAGS);
  RegisterValue::FlagsType exp1 = {
      {true, "HELLO", 0},
      {false, "WORLD", 31},
      {false, "HELLO2", 32},
      {true, "WORLD2", 63}};
  EXPECT_EQ(val.value.flagsValue, exp1);
}

TEST(RegisterValueTest, CopyMoveTest) {
  RegisterDescriptor d;
  d.format = RegisterValueType::STRING;
  RegisterValue val(
      {0x3730, 0x302d, 0x3031, 0x3436, 0x3731, 0x2d30, 0x3030, 0x3020},
      d,
      0x12345678);
  RegisterValue val2(val);
  EXPECT_EQ(val2.type, RegisterValueType::STRING);
  EXPECT_EQ(val2.value.strValue, val.value.strValue);
  RegisterValue val3(std::move(val2));
  EXPECT_EQ(val3.type, RegisterValueType::STRING);
  EXPECT_EQ(val3.value.strValue, val.value.strValue);
  // Ensure we are moving from 2 and not just copying.
  // XXX move symantics technically allows only destructor and a copy from a
  // moved object, but it supposedly leaves it in a semi-valid state, so
  // checking length for 0 seems a decent option.
  EXPECT_EQ(val2.value.strValue.length(), 0);
}
