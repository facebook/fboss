// Copyright 2021-present Facebook. All Rights Reserved.
#include "TestUtils.h"

#if (defined(__llvm__) && (__clang_major__ < 9)) || \
    (!defined(__llvm__) && (__GNUC__ < 8))
#include <experimental/filesystem>
namespace std {
namespace filesystem = experimental::filesystem;
}
#else
#include <filesystem>
#endif
#include "DeviceLocationIterator.h"
#include "Register.h"

using namespace std;
using namespace testing;
using namespace rackmon;
using nlohmann::json;

TEST(DeviceLocationIteratorTest, Basic) {
  std::string json1 = R"(
  {
    "name": "orv2_psu",
    "address_range": [[160, 162], [10, 10]],
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

  RegisterMapDatabase db;
  db.load(nlohmann::json::parse(json1));

  json exp = R"({
    "device_path": "/tmp/blah",
    "baudrate": 19200,
    "port": 123
  })"_json;

  std::vector<std::unique_ptr<Modbus>> interfaces;
  EXPECT_THROW(*DeviceLocationIterator(db, interfaces), runtime_error);
  interfaces.push_back(std::make_unique<Modbus>());
  interfaces.back()->initialize(exp);
  EXPECT_EQ(interfaces.back().get()->getPort().value(), 123);

  {
    DeviceLocationIterator iterator(db, interfaces);
    EXPECT_EQ((*iterator).addr, 160);
    EXPECT_EQ((*iterator).interface.getPort(), optional<uint8_t>(123));
    ++iterator;

    EXPECT_EQ((*iterator).addr, 161);
    EXPECT_EQ((*iterator).interface.getPort(), optional<uint8_t>(123));
    ++iterator;

    EXPECT_EQ((*iterator).addr, 162);
    EXPECT_EQ((*iterator).interface.getPort(), optional<uint8_t>(123));
    ++iterator;

    EXPECT_EQ((*iterator).addr, 10);
    EXPECT_EQ((*iterator).interface.getPort(), optional<uint8_t>(123));
    ++iterator;

    EXPECT_THROW(*iterator, out_of_range);
  }

  json exp_no_port = R"({
        "device_path": "/tmp/blah",
        "baudrate": 19200
      })"_json;
  interfaces.push_back(std::make_unique<Modbus>());
  interfaces.back()->initialize(exp_no_port);
  EXPECT_FALSE(interfaces.back().get()->getPort().has_value());

  {
    DeviceLocationIterator iterator(db, interfaces);

    EXPECT_EQ((*iterator).addr, 160);
    EXPECT_EQ((*iterator).interface.getPort(), optional<uint8_t>(123));
    ++iterator;

    EXPECT_EQ((*iterator).addr, 160);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_EQ((*iterator).addr, 161);
    EXPECT_EQ((*iterator).interface.getPort(), optional<uint8_t>(123));
    ++iterator;

    EXPECT_EQ((*iterator).addr, 161);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_EQ((*iterator).addr, 162);
    EXPECT_EQ((*iterator).interface.getPort(), optional<uint8_t>(123));
    ++iterator;

    EXPECT_EQ((*iterator).addr, 162);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_EQ((*iterator).addr, 10);
    EXPECT_EQ((*iterator).interface.getPort(), optional<uint8_t>(123));
    ++iterator;

    EXPECT_EQ((*iterator).addr, 10);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_THROW(*iterator, out_of_range);
  }

  std::string json2 = R"(
        {
          "name": "orv2_psu",
          "address_range": [[1, 2], [3, 4]],
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
  db.load(nlohmann::json::parse(json2));

  {
    DeviceLocationIterator iterator(db, interfaces);

    EXPECT_EQ((*iterator).addr, 160);
    EXPECT_EQ((*iterator).interface.getPort(), optional<uint8_t>(123));
    ++iterator;

    EXPECT_EQ((*iterator).addr, 160);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_EQ((*iterator).addr, 161);
    EXPECT_EQ((*iterator).interface.getPort(), optional<uint8_t>(123));
    ++iterator;

    EXPECT_EQ((*iterator).addr, 161);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_EQ((*iterator).addr, 162);
    EXPECT_EQ((*iterator).interface.getPort(), optional<uint8_t>(123));
    ++iterator;

    EXPECT_EQ((*iterator).addr, 162);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_EQ((*iterator).addr, 10);
    EXPECT_EQ((*iterator).interface.getPort(), optional<uint8_t>(123));
    ++iterator;

    EXPECT_EQ((*iterator).addr, 10);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_EQ((*iterator).addr, 1);
    EXPECT_EQ((*iterator).interface.getPort(), optional<uint8_t>(123));
    ++iterator;

    EXPECT_EQ((*iterator).addr, 1);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_EQ((*iterator).addr, 2);
    EXPECT_EQ((*iterator).interface.getPort(), optional<uint8_t>(123));
    ++iterator;

    EXPECT_EQ((*iterator).addr, 2);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_EQ((*iterator).addr, 3);
    EXPECT_EQ((*iterator).interface.getPort(), optional<uint8_t>(123));
    ++iterator;

    EXPECT_EQ((*iterator).addr, 3);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_EQ((*iterator).addr, 4);
    EXPECT_EQ((*iterator).interface.getPort(), optional<uint8_t>(123));
    ++iterator;

    EXPECT_EQ((*iterator).addr, 4);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_THROW(*iterator, out_of_range);
    EXPECT_EQ(iterator, iterator.end());
  }

  {
    DeviceLocationIterator iterator(db, interfaces);
    int count = 0;
    while (iterator != iterator.end()) {
      EXPECT_NO_THROW(*iterator);
      ++iterator;
      count++;
    }
    EXPECT_EQ(count, 16);
    EXPECT_THROW(*iterator, out_of_range);
    EXPECT_THROW(++iterator, out_of_range);
  }
}
