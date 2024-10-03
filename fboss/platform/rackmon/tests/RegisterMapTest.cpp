// Copyright 2021-present Facebook. All Rights Reserved.
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <fstream>
#if (defined(__llvm__) && (__clang_major__ < 9)) || \
    (!defined(__llvm__) && (__GNUC__ < 8))
#include <experimental/filesystem>
namespace std {
namespace filesystem = experimental::filesystem;
}
#else
#include <filesystem>
#endif
#include "Register.h"

using namespace std;
using namespace testing;
using namespace rackmon;

//--------------------------------------------------------
// Address Range Tests
//--------------------------------------------------------

TEST(AddrRangeTest, Basic) {
  AddrRange a({{10, 20}, {25, 30}});
  EXPECT_TRUE(a.contains(10));
  EXPECT_TRUE(a.contains(11));
  EXPECT_TRUE(a.contains(20));
  EXPECT_TRUE(a.contains(19));
  EXPECT_TRUE(a.contains(25));
  EXPECT_TRUE(a.contains(30));
  EXPECT_FALSE(a.contains(9));
  EXPECT_FALSE(a.contains(21));
  EXPECT_FALSE(a.contains(24));
  EXPECT_FALSE(a.contains(31));
}

TEST(RegisterMapTest, JSONCoversion) {
  std::string inp = R"({
    "name": "orv2_psu",
    "address_range": [[160, 191]],
    "probe_register": 104,
    "baudrate": 19200,
    "registers": [
      {
        "begin": 0,
        "length": 8,
        "format": "STRING",
        "name": "MFG_MODEL"
      },
      {
          "begin": 127,
          "length": 1,
          "keep": 10,
          "format": "FLOAT",
          "precision": 6,
          "name": "BBU Absolute State of Charge"
      }
    ]
  })";
  nlohmann::json j = nlohmann::json::parse(inp);
  RegisterMap rmap = j;
  EXPECT_TRUE(std::any_of(
      rmap.applicableAddresses.range.cbegin(),
      rmap.applicableAddresses.range.cend(),
      [](auto const& ent) { return (ent.first == 160 && ent.second == 191); }));
  EXPECT_EQ(rmap.probeRegister, 104);
  EXPECT_EQ(rmap.baudrate, 19200);
  EXPECT_EQ(rmap.name, "orv2_psu");
  EXPECT_EQ(rmap.registerDescriptors.size(), 2);
  EXPECT_EQ(rmap.specialHandlers.size(), 0);
  EXPECT_EQ(rmap.at(0).begin, 0);
  EXPECT_EQ(rmap.at(0).length, 8);
  EXPECT_EQ(rmap.at(0).format, RegisterValueType::STRING);
  EXPECT_EQ(rmap.at(0).name, "MFG_MODEL");
  EXPECT_EQ(rmap.at(0).keep, 1);
  EXPECT_EQ(rmap.at(0).storeChangesOnly, false);
  EXPECT_EQ(rmap.at(127).begin, 127);
  EXPECT_EQ(rmap.at(127).length, 1);
  EXPECT_EQ(rmap.at(127).format, RegisterValueType::FLOAT);
  EXPECT_EQ(rmap.at(127).name, "BBU Absolute State of Charge");
  EXPECT_EQ(rmap.at(127).keep, 10);
  EXPECT_EQ(rmap.at(127).storeChangesOnly, false);
  EXPECT_EQ(rmap.at(127).precision, 6);
  EXPECT_THROW(rmap.at(42), std::out_of_range);
}

TEST(RegisterMapTest, JSONCoversionBaudrate) {
  std::string inp = R"({
    "name": "orv2_psu",
    "address_range": [[160, 191]],
    "probe_register": 104,
    "baudrate": 19200,
    "registers": [
      {
        "begin": 0,
        "length": 8,
        "format": "STRING",
        "name": "MFG_MODEL"
      }
    ]
  })";
  nlohmann::json j = nlohmann::json::parse(inp);
  RegisterMap rmap = j;
  EXPECT_TRUE(std::any_of(
      rmap.applicableAddresses.range.cbegin(),
      rmap.applicableAddresses.range.cend(),
      [](auto const& ent) { return (ent.first == 160 && ent.second == 191); }));
  EXPECT_EQ(rmap.probeRegister, 104);
  EXPECT_EQ(rmap.baudrate, 19200);
  EXPECT_EQ(rmap.name, "orv2_psu");
  EXPECT_EQ(rmap.registerDescriptors.size(), 1);
  EXPECT_EQ(rmap.specialHandlers.size(), 0);
}

TEST(RegisterMapTest, JSONCoversionSpecial) {
  std::string inp = R"({
    "name": "orv2_psu",
    "address_range": [[160, 191]],
    "probe_register": 104,
    "baudrate": 19200,
    "special_handlers": [
      {
        "reg": 298,
        "len": 2,
        "period": 3600,
        "action": "write",
        "info": {
          "interpret": "INTEGER",
          "shell": "date +%s"
        }
      }
    ],
    "registers": [
      {
        "begin": 0,
        "length": 8,
        "format": "STRING",
        "name": "MFG_MODEL"
      },
      {
          "begin": 127,
          "length": 1,
          "keep": 10,
          "format": "FLOAT",
          "precision": 6,
          "name": "BBU Absolute State of Charge"
      }
    ]
  })";
  nlohmann::json j = nlohmann::json::parse(inp);
  RegisterMap rmap = j;
  EXPECT_TRUE(std::any_of(
      rmap.applicableAddresses.range.cbegin(),
      rmap.applicableAddresses.range.cend(),
      [](auto const& ent) { return (ent.first == 160 && ent.second == 191); }));
  EXPECT_EQ(rmap.probeRegister, 104);
  EXPECT_EQ(rmap.baudrate, 19200);
  EXPECT_EQ(rmap.name, "orv2_psu");
  EXPECT_EQ(rmap.registerDescriptors.size(), 2);
  EXPECT_EQ(rmap.specialHandlers.size(), 1);
  EXPECT_EQ(rmap.specialHandlers[0].reg, 0x12A);
  EXPECT_EQ(rmap.specialHandlers[0].len, 2);
  EXPECT_EQ(rmap.specialHandlers[0].period, 3600);
  EXPECT_EQ(rmap.specialHandlers[0].action, "write");
  EXPECT_EQ(rmap.specialHandlers[0].info.interpret, RegisterValueType::INTEGER);
  EXPECT_TRUE(rmap.specialHandlers[0].info.shell);
  EXPECT_FALSE(rmap.specialHandlers[0].info.value);
  EXPECT_EQ(rmap.specialHandlers[0].info.shell.value(), R"(date +%s)");
}

class RegisterMapDatabaseTest : public ::testing::Test {
 public:
  const std::string r_test_dir = "./test_rackmon.d";
  const std::string r_test1 = r_test_dir + "/test1.json";
  const std::string r_test2 = r_test_dir + "/test2.json";
  std::string json1{};
  std::string json2{};

 protected:
  void SetUp() override {
    mkdir(r_test_dir.c_str(), 0755);
    json1 = R"({
        "name": "orv2_psu",
        "address_range": [[160, 191], [10, 10]],
        "probe_register": 104,
        "baudrate": 19200,
        "registers": [
          {
            "begin": 0,
            "length": 8,
            "format": "STRING",
            "name": "MFG_MODEL",
            "interval": 40
          }
        ]
      })";
    json2 = R"({
        "name": "orv3_psu",
        "address_range": [[110, 140], [10, 10]],
        "probe_register": 104,
        "baudrate": 115200,
        "registers": [
          {
            "begin": 0,
            "length": 8,
            "format": "STRING",
            "name": "MFG_MODEL"
          }
        ]
      })";
    std::ofstream ofs1(r_test1);
    ofs1 << json1;
    ofs1.close();
    std::ofstream ofs2(r_test2);
    ofs2 << json2;
    ofs2.close();
  }
  void TearDown() override {
    remove(r_test1.c_str());
    remove(r_test2.c_str());
    remove(r_test_dir.c_str());
  }
};

TEST_F(RegisterMapDatabaseTest, Load) {
  RegisterMapDatabase db;
  EXPECT_THROW(db.at(160), std::out_of_range);
  db.load(nlohmann::json::parse(json1));
  EXPECT_THROW(db.at(42), std::out_of_range);
  const auto& m1 = db.at(160);
  EXPECT_EQ(m1.name, "orv2_psu");
  const auto& m2 = db.at(164);
  EXPECT_EQ(m2.name, "orv2_psu");
  const auto& m3 = db.at(191);
  EXPECT_EQ(m3.name, "orv2_psu");
  EXPECT_THROW(db.at(159), std::out_of_range);
  EXPECT_THROW(db.at(192), std::out_of_range);
  EXPECT_THROW(db.at(120), std::out_of_range);
  db.load(nlohmann::json::parse(json2));
  const auto& m4 = db.at(110);
  EXPECT_EQ(m4.name, "orv3_psu");
  EXPECT_THROW(db.at(109), std::out_of_range);
  EXPECT_THROW(db.at(141), std::out_of_range);
  EXPECT_THROW(db.at(159), std::out_of_range);
  EXPECT_THROW(db.at(192), std::out_of_range);
  const auto& m5 = db.at(164);
  EXPECT_EQ(m5.name, "orv2_psu");
  const auto& m6 = db.at(140);
  EXPECT_EQ(m6.name, "orv3_psu");
  const auto& m7 = db.at(120);
  EXPECT_EQ(m7.name, "orv3_psu");
  EXPECT_EQ(db.minMonitorInterval(), 40);
  auto it = db.find(10);
  EXPECT_NE(it, db.end());
  const auto& m8 = *it;
  EXPECT_EQ(m8.name, "orv2_psu");
  ++it;
  EXPECT_NE(it, db.end());
  const auto& m9 = *it;
  EXPECT_EQ(m9.name, "orv3_psu");
  ++it;
  EXPECT_EQ(it, db.end());
  EXPECT_THROW(*it, std::out_of_range);
}
