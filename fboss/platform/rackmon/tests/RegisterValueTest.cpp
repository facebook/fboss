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
  EXPECT_EQ(std::get<std::vector<uint8_t>>(val.value), exp);

  nlohmann::json j = val;
  EXPECT_TRUE(j.is_object());
  EXPECT_TRUE(j.contains("type") && j["type"].is_string());
  EXPECT_TRUE(j.contains("timestamp") && j["timestamp"].is_number_integer());
  EXPECT_TRUE(
      j.contains("value") && j["value"].contains("rawValue") &&
      j["value"]["rawValue"].is_array());
  EXPECT_EQ(std::string(j["type"]), "RAW");
  EXPECT_EQ(j["timestamp"], 0);
  EXPECT_EQ(j["value"]["rawValue"].size(), 4);
  EXPECT_EQ(std::vector<uint8_t>(j["value"]["rawValue"]), exp);
}

TEST(RegisterValueTest, STRING) {
  RegisterDescriptor d;
  d.format = RegisterValueType::STRING;
  RegisterValue val(
      {0x3730, 0x302d, 0x3031, 0x3436, 0x3731, 0x2d30, 0x3030, 0x3020},
      d,
      0x12345678);
  EXPECT_EQ(val.type, RegisterValueType::STRING);
  EXPECT_EQ(std::get<std::string>(val.value), "700-014671-0000 ");
  EXPECT_EQ(val.timestamp, 0x12345678);

  nlohmann::json j = val;
  EXPECT_TRUE(j.is_object());
  EXPECT_TRUE(j.contains("type") && j["type"].is_string());
  EXPECT_TRUE(j.contains("timestamp") && j["timestamp"].is_number_integer());
  EXPECT_TRUE(
      j.contains("value") && j["value"].contains("strValue") &&
      j["value"]["strValue"].is_string());
  EXPECT_EQ(std::string(j["type"]), "STRING");
  EXPECT_EQ(j["timestamp"], 0x12345678);
  EXPECT_EQ(j["value"]["strValue"], "700-014671-0000 ");
}

TEST(RegisterValueTest, INTEGER) {
  RegisterDescriptor d;
  d.format = RegisterValueType::INTEGER;
  RegisterValue val({0x1234, 0x5678}, d, 0x12345678);
  EXPECT_EQ(val.type, RegisterValueType::INTEGER);
  EXPECT_EQ(std::get<int32_t>(val.value), 0x12345678);

  nlohmann::json j = val;
  EXPECT_TRUE(j.is_object());
  EXPECT_TRUE(j.contains("type") && j["type"].is_string());
  EXPECT_TRUE(j.contains("timestamp") && j["timestamp"].is_number_integer());
  EXPECT_TRUE(
      j.contains("value") && j["value"].contains("intValue") &&
      j["value"]["intValue"].is_number_integer());
  EXPECT_EQ(std::string(j["type"]), "INTEGER");
  EXPECT_EQ(j["timestamp"], 0x12345678);
  EXPECT_EQ(j["value"]["intValue"], 0x12345678);
}

TEST(RegisterValueTest, LITTLE_INTEGER) {
  RegisterDescriptor d;
  d.format = RegisterValueType::INTEGER;
  d.endian = RegisterEndian::LITTLE;
  RegisterValue val({0x1234, 0x5678}, d, 0x12345678);
  EXPECT_EQ(val.type, RegisterValueType::INTEGER);
  EXPECT_EQ(std::get<int32_t>(val.value), 0x78563412);
}

TEST(RegisterValueTest, FLOAT) {
  RegisterDescriptor d;
  d.format = RegisterValueType::FLOAT;
  d.precision = 11;
  RegisterValue val({0x64fc}, d, 0x12345678);
  EXPECT_EQ(val.type, RegisterValueType::FLOAT);
  EXPECT_NEAR(std::get<float>(val.value), 12.623, 0.001);

  nlohmann::json j = val;
  EXPECT_TRUE(j.is_object());
  EXPECT_TRUE(j.contains("type") && j["type"].is_string());
  EXPECT_TRUE(j.contains("timestamp") && j["timestamp"].is_number_integer());
  EXPECT_TRUE(
      j.contains("value") && j["value"].contains("floatValue") &&
      j["value"]["floatValue"].is_number_float());
  EXPECT_EQ(std::string(j["type"]), "FLOAT");
  EXPECT_EQ(j["timestamp"], 0x12345678);
  EXPECT_NEAR(j["value"]["floatValue"], 12.623, 0.001);
}

TEST(RegisterValueTest, FLAGS) {
  RegisterDescriptor d;
  d.format = RegisterValueType::FLAGS;
  d.flags = {{0, "HELLO"}, {1, "WORLD"}};
  RegisterValue val1({0x0003}, d, 0x12345678);
  EXPECT_EQ(val1.type, RegisterValueType::FLAGS);
  std::string expstr1 = "\n*[1] <0> HELLO\n*[1] <1> WORLD";
  RegisterValue::FlagsType exp1 = {{true, "HELLO", 0}, {true, "WORLD", 1}};
  EXPECT_EQ(std::get<RegisterValue::FlagsType>(val1.value), exp1);

  RegisterValue val2({0x0000}, d, 0x12345678);
  EXPECT_EQ(val1.type, RegisterValueType::FLAGS);
  RegisterValue::FlagsType exp2 = {{false, "HELLO", 0}, {false, "WORLD", 1}};
  EXPECT_EQ(std::get<RegisterValue::FlagsType>(val2.value), exp2);

  RegisterValue val3({0x0002}, d, 0x12345678);
  EXPECT_EQ(val3.type, RegisterValueType::FLAGS);
  RegisterValue::FlagsType exp3 = {{false, "HELLO", 0}, {true, "WORLD", 1}};
  EXPECT_EQ(std::get<RegisterValue::FlagsType>(val3.value), exp3);

  nlohmann::json j = val3;
  EXPECT_TRUE(j.is_object());
  EXPECT_TRUE(j.contains("type") && j["type"].is_string());
  EXPECT_TRUE(j.contains("timestamp") && j["timestamp"].is_number_integer());
  EXPECT_TRUE(
      j.contains("value") && j["value"].contains("flagsValue") &&
      j["value"]["flagsValue"].is_array());
  EXPECT_EQ(std::string(j["type"]), "FLAGS");
  EXPECT_EQ(j["timestamp"], 0x12345678);
  EXPECT_EQ(j["value"]["flagsValue"].size(), 2);
  EXPECT_TRUE(j["value"]["flagsValue"][0].is_array());
  EXPECT_TRUE(j["value"]["flagsValue"][1].is_array());
  EXPECT_EQ(j["value"]["flagsValue"][0].size(), 3);
  EXPECT_EQ(j["value"]["flagsValue"][1].size(), 3);
  EXPECT_TRUE(j["value"]["flagsValue"][0][0].is_boolean());
  EXPECT_TRUE(j["value"]["flagsValue"][1][0].is_boolean());
  EXPECT_TRUE(j["value"]["flagsValue"][0][1].is_string());
  EXPECT_TRUE(j["value"]["flagsValue"][1][1].is_string());
  EXPECT_FALSE(j["value"]["flagsValue"][0][0]);
  EXPECT_TRUE(j["value"]["flagsValue"][1][0]);
  EXPECT_EQ(std::string(j["value"]["flagsValue"][0][1]), "HELLO");
  EXPECT_EQ(std::string(j["value"]["flagsValue"][1][1]), "WORLD");
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
  EXPECT_EQ(std::get<RegisterValue::FlagsType>(val.value), exp1);
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
  EXPECT_EQ(val2.value, val.value);
  RegisterValue val3(std::move(val2));
  EXPECT_EQ(val3.type, RegisterValueType::STRING);
  EXPECT_EQ(val3.value, val.value);
  // Ensure we are moving from 2 and not just copying.
  // XXX move symantics technically allows only destructor and a copy from a
  // moved object, but it supposedly leaves it in a semi-valid state, so
  // checking length for 0 seems a decent option.
  EXPECT_EQ(std::get<std::string>(val2.value).length(), 0);
}

TEST(RegisterValueTest, AsignmentTest) {
  RegisterDescriptor d1, d2;
  d1.format = RegisterValueType::STRING;
  d2.format = RegisterValueType::INTEGER;
  RegisterValue strVal(
      {0x3730, 0x302d, 0x3031, 0x3436, 0x3731, 0x2d30, 0x3030, 0x3020},
      d1,
      0x12345678);
  RegisterValue intVal({0x1}, d2, 0x56781234);
  RegisterValue val;
  val = strVal;
  EXPECT_EQ(val.type, RegisterValueType::STRING);
  EXPECT_EQ(val.value, strVal.value);
  EXPECT_EQ(val.timestamp, strVal.timestamp);
  val = intVal;
  EXPECT_EQ(val.type, RegisterValueType::INTEGER);
  EXPECT_EQ(val.value, intVal.value);
  EXPECT_EQ(val.timestamp, intVal.timestamp);
}
