// Copyright 2021-present Facebook. All Rights Reserved.
#include "Rackmon.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <fstream>
#include "TempDir.h"

using namespace std;
using namespace testing;
using nlohmann::json;
using namespace rackmon;

class FakeModbus : public Modbus {
  uint8_t exp_addr;
  uint8_t min_addr;
  uint8_t max_addr;
  uint32_t baud;
  bool probed = false;
  Encoder encoder{};

 public:
  FakeModbus(uint8_t e, uint8_t mina, uint8_t maxa, uint32_t b)
      : Modbus(), exp_addr(e), min_addr(mina), max_addr(maxa), baud(b) {}
  void command(
      Msg& req,
      Msg& resp,
      uint32_t b,
      ModbusTime /* unused */,
      Parity /* unused */) override {
    encoder.encode(req);
    EXPECT_GE(req.addr, min_addr);
    EXPECT_LE(req.addr, max_addr);
    // We are mocking a system with only one available
    // address, exp_addr. So, throw an exception for others.
    if (req.addr != exp_addr || b != baud) {
      throw TimeoutException();
    }
    // There is really no reason at this point for rackmon
    // to be sending any message other than read-holding-regs
    // TODO When adding support for baudrate negotitation etc
    // we might need to make this more flexible.
    EXPECT_EQ(req.raw[1], 0x3);
    EXPECT_EQ(req.len, 8);
    EXPECT_EQ(req.raw[2], 0);
    EXPECT_EQ(req.raw[4], 0);
    if (probed) {
      Msg expMsg = 0x000300000008_M;
      expMsg.addr = exp_addr;
      Encoder::finalize(expMsg);
      EXPECT_EQ(req, expMsg);
      resp = 0x0003106162636465666768696a6b6c6d6e6f70_M;
    } else {
      Msg expMsg = 0x000300680001_M;
      expMsg.addr = exp_addr;
      Encoder::finalize(expMsg);
      EXPECT_EQ(req, expMsg);
      // Allow to be probed only once.
      probed = true;
      resp = 0x0003020000_M;
    }
    resp.addr = exp_addr;
    Encoder::finalize(resp);
    Encoder::decode(resp);
  }
};

class Mock3Modbus : public Modbus {
 public:
  Mock3Modbus(uint8_t e, uint8_t mina, uint8_t maxa, uint32_t b)
      : Modbus(), fake_(e, mina, maxa, b) {
    ON_CALL(*this, command(_, _, _, _, _))
        .WillByDefault(Invoke([this](
                                  Msg& req,
                                  Msg& resp,
                                  uint32_t b,
                                  ModbusTime timeout,
                                  Parity parity) {
          return fake_.command(req, resp, b, timeout, parity);
        }));
  }
  MOCK_METHOD0(isPresent, bool());
  MOCK_METHOD1(initialize, void(const nlohmann::json& j));
  MOCK_METHOD5(command, void(Msg&, Msg&, uint32_t, ModbusTime, Parity));

  FakeModbus fake_;
};

class MockRackmon : public Rackmon {
 public:
  MockRackmon() : Rackmon() {}
  MOCK_METHOD0(makeInterface, std::unique_ptr<Modbus>());
  MOCK_METHOD0(getTime, time_t());
  const RegisterMapDatabase& getMap() {
    return getRegisterMapDatabase();
  }
  void scanTick() {
    getScanThread().tick();
  }
  void monitorTick() {
    getMonitorThread().tick();
  }
};

class RackmonTest : public ::testing::Test {
  TempDirectory test_map_dir{};
  TempDirectory test_dir{};

 public:
  std::string r_test_dir{};
  std::string r_conf{};
  std::string r_test1_map{};
  std::string r_test2_map{};

 public:
  void SetUp() override {
    r_test_dir = test_map_dir.path();
    r_conf = test_dir.path() + "/rackmon.conf";
    r_test1_map = r_test_dir + "/test1.json";
    r_test2_map = r_test_dir + "/test2.json";

    std::string json1 = R"({
        "name": "orv2_psu",
        "address_range": [[160, 162]],
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
    std::ofstream ofs1(r_test1_map);
    ofs1 << json1;
    ofs1.close();

    std::string rconf_s = R"({
      "interfaces": [
        {
          "device_path": "/tmp/blah",
          "baudrate": 19200
        }
      ]
    })";
    std::ofstream ofs2(r_conf);
    ofs2 << rconf_s;
    ofs2.close();
  }

 public:
  std::unique_ptr<Modbus>
  make_modbus(uint8_t exp_addr, int num_cmd_calls, uint32_t exp_baud = 19200) {
    json exp = R"({
      "device_path": "/tmp/blah",
      "baudrate": 19200
    })"_json;
    std::unique_ptr<Mock3Modbus> ptr =
        std::make_unique<Mock3Modbus>(exp_addr, 160, 162, exp_baud);
    EXPECT_CALL(*ptr, initialize(exp)).Times(1);
    if (num_cmd_calls > 0) {
      EXPECT_CALL(*ptr, isPresent())
          .Times(AtLeast(3))
          .WillRepeatedly(Return(true));
      EXPECT_CALL(*ptr, command(_, _, _, _, _)).Times(AtLeast(num_cmd_calls));
    }
    std::unique_ptr<Modbus> ptr2 = std::move(ptr);
    return ptr2;
  }
};

TEST_F(RackmonTest, BasicLoad) {
  std::string json2 = R"({
      "name": "orv3_psu",
      "address_range": [[110, 112]],
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
  std::ofstream ofs2(r_test2_map);
  ofs2 << json2;
  ofs2.close();

  MockRackmon mon;
  EXPECT_CALL(mon, makeInterface())
      .Times(1)
      .WillOnce(Return(ByMove(make_modbus(0, 0))));
  mon.load(r_conf, r_test_dir);
  const RegisterMapDatabase& db = mon.getMap();
  const auto& m1 = db.at(110);
  EXPECT_EQ(m1.name, "orv3_psu");
  const auto& m2 = db.at(112);
  EXPECT_EQ(m2.name, "orv3_psu");
  const auto& m3 = db.at(111);
  EXPECT_EQ(m3.name, "orv3_psu");
  const auto& m4 = db.at(160);
  EXPECT_EQ(m4.name, "orv2_psu");
  const auto& m5 = db.at(162);
  EXPECT_EQ(m5.name, "orv2_psu");
  const auto& m6 = db.at(161);
  EXPECT_EQ(m6.name, "orv2_psu");
  EXPECT_THROW(db.at(109), std::out_of_range);
  EXPECT_THROW(db.at(113), std::out_of_range);
  EXPECT_THROW(db.at(159), std::out_of_range);
  EXPECT_THROW(db.at(163), std::out_of_range);
}

TEST_F(RackmonTest, BasicScanFoundNone) {
  MockRackmon mon;
  // Mock a modbus with no active devices,
  // we expect rackmon to scan all of them on
  // start up.
  EXPECT_CALL(mon, makeInterface())
      .Times(1)
      .WillOnce(Return(ByMove(make_modbus(0, 3))));
  mon.load(r_conf, r_test_dir);
  mon.start();
  std::vector<ModbusDeviceInfo> devs = mon.listDevices();
  EXPECT_EQ(devs.size(), 0);
  mon.stop(false);
  Request req;
  Response resp;
  req.raw[0] = 100; // Some unknown address, this should throw
  req.raw[1] = 0x3;
  req.len = 2;
  EXPECT_THROW(mon.rawCmd(req, resp, 1s), std::out_of_range);
}

TEST_F(RackmonTest, BasicScanFoundOne) {
  std::string json2 = R"({
      "name": "orv3_psu",
      "address_range": [[161, 161]],
      "probe_register": 104,
      "baudrate": 115200,
      "registers": [
        {
          "begin": 0,
          "length": 8,
          "format": "STRING",
          "name": "MFG_MODEL2"
        }
      ]
    })";
  std::ofstream ofs2(r_test2_map);
  ofs2 << json2;
  ofs2.close();

  MockRackmon mon;
  // Mock a modbus with no active devices,
  // we expect rackmon to scan all of them on
  // start up.
  EXPECT_CALL(mon, makeInterface())
      .Times(1)
      .WillOnce(Return(ByMove(make_modbus(161, 4, 115200))));
  mon.load(r_conf, r_test_dir);
  mon.start();

  // Fake elapsed time of one interval.
  mon.scanTick();
  std::vector<ModbusDeviceInfo> devs = mon.listDevices();
  EXPECT_EQ(devs.size(), 1);
  EXPECT_EQ(devs[0].baudrate, 115200);
  EXPECT_EQ(devs[0].deviceAddress, 161);
  EXPECT_EQ(devs[0].deviceType, "orv3_psu");
  EXPECT_EQ(devs[0].mode, ModbusDeviceMode::ACTIVE);
  mon.stop();

  Request rreq;
  Response rresp;
  rreq.raw[0] = 100; // Some unknown address, this should throw
  rreq.raw[1] = 0x3;
  rreq.len = 2;
  std::vector<uint16_t> read_regs;
  EXPECT_THROW(mon.rawCmd(rreq, rresp, 1s), std::out_of_range);

  // Only difference of implementations between Rackmon and ModbusDevice
  // is rackmon checks validity of address. Check that we are throwing
  // correctly. The actual functionality is tested in modbus_device_test.cpp.
  EXPECT_THROW(
      mon.readHoldingRegisters(100, 0x123, read_regs), std::out_of_range);
  EXPECT_THROW(mon.writeSingleRegister(100, 0x123, 0x1234), std::out_of_range);
  EXPECT_THROW(
      mon.writeMultipleRegisters(100, 0x123, read_regs), std::out_of_range);
  std::vector<FileRecord> records(1);
  records[0].data.resize(2);
  EXPECT_THROW(mon.readFileRecord(100, records), std::out_of_range);

  // Use a known handled response.
  ReadHoldingRegistersReq req(161, 0, 8);
  std::vector<uint16_t> regs(8);
  ReadHoldingRegistersResp resp(161, regs);
  mon.rawCmd(req, resp, 1s);

  EXPECT_EQ(regs[0], 'a' << 8 | 'b');
}

TEST_F(RackmonTest, BasicScanFoundOneMon) {
  MockRackmon mon;
  // Mock a modbus with no active devices,
  // we expect rackmon to scan all of them on
  // start up.
  EXPECT_CALL(mon, makeInterface())
      .Times(1)
      .WillOnce(Return(ByMove(make_modbus(161, 4))));
  mon.load(r_conf, r_test_dir);
  mon.start();

  // Fake that a tick has elapsed on scan's pollthread.
  mon.scanTick();
  std::vector<ModbusDeviceInfo> devs = mon.listDevices();
  EXPECT_EQ(devs.size(), 1);
  EXPECT_EQ(devs[0].deviceAddress, 161);
  EXPECT_EQ(devs[0].mode, ModbusDeviceMode::ACTIVE);

  // Fake that a tick has elapsed on monitor's pollthread.
  mon.monitorTick();
  mon.stop(false);
  std::vector<ModbusDeviceValueData> data;
  mon.getValueData(data);
  EXPECT_EQ(data.size(), 1);
  EXPECT_EQ(data[0].deviceType, "orv2_psu");
  EXPECT_EQ(data[0].registerList.size(), 1);
  EXPECT_EQ(data[0].registerList[0].regAddr, 0);
  EXPECT_EQ(data[0].registerList[0].name, "MFG_MODEL");
  EXPECT_EQ(data[0].registerList[0].history.size(), 1);
  EXPECT_EQ(data[0].registerList[0].history[0].type, RegisterValueType::STRING);
  EXPECT_EQ(
      std::get<std::string>(data[0].registerList[0].history[0].value),
      "abcdefghijklmnop");
  EXPECT_NEAR(
      data[0].registerList[0].history[0].timestamp, std::time(nullptr), 10);

  ModbusDeviceFilter filter1, filter2, filter3, filter4;
  ModbusRegisterFilter rFilter1, rFilter2;
  filter1.addrFilter = {161, 162};
  filter2.addrFilter = {162};
  filter3.typeFilter = {"orv2_psu"};
  filter4.typeFilter = {"orv3_psu"};
  rFilter1.addrFilter = {0, 10};
  rFilter2.addrFilter = {10};

  mon.getValueData(data, filter1);
  EXPECT_EQ(data.size(), 1);
  EXPECT_EQ(data[0].registerList.size(), 1);

  mon.getValueData(data, filter2);
  EXPECT_EQ(data.size(), 0);

  mon.getValueData(data, filter3);
  EXPECT_EQ(data.size(), 1);

  mon.getValueData(data, filter4);
  EXPECT_EQ(data.size(), 0);

  mon.getValueData(data, {}, rFilter1);
  EXPECT_EQ(data.size(), 1);
  EXPECT_EQ(data[0].registerList.size(), 1);
  EXPECT_EQ(data[0].registerList[0].regAddr, 0);

  mon.getValueData(data, {}, rFilter2, true);
  EXPECT_EQ(data.size(), 1);
  EXPECT_EQ(data[0].registerList.size(), 0);
}

TEST_F(RackmonTest, DormantRecovery) {
  std::atomic<bool> commandTimeout{false};
  MockRackmon mon;
  auto make_flaky = [&commandTimeout]() {
    json exp = R"({
      "device_path": "/tmp/blah",
      "baudrate": 19200
    })"_json;
    std::unique_ptr<Mock3Modbus> ptr =
        std::make_unique<Mock3Modbus>(161, 160, 162, 19200);
    EXPECT_CALL(*ptr, initialize(exp)).Times(1);
    EXPECT_CALL(*ptr, isPresent()).WillRepeatedly(Return(true));
    EXPECT_CALL(*ptr, command(_, _, _, _, _))
        .WillRepeatedly(
            Invoke([&commandTimeout](Msg&, Msg&, uint32_t, ModbusTime, Parity) {
              if (commandTimeout) {
                throw TimeoutException();
              }
            }));
    std::unique_ptr<Modbus> ptr2 = std::move(ptr);
    return ptr2;
  };
  // Mock a modbus with no active devices,
  // we expect rackmon to scan all of them on
  // start up.
  EXPECT_CALL(mon, makeInterface())
      .Times(1)
      .WillOnce(Return(ByMove(make_flaky())));
  json ifaceConfig = R"({
    "interfaces": [
      {
        "device_path": "/tmp/blah",
        "baudrate": 19200
      }
    ]
  })"_json;
  json regmapConfig = R"({
    "name": "orv2_psu",
    "address_range": [[161, 161]],
    "probe_register": 104,
    "baudrate": 19200,
    "registers": [
      {
        "begin": 0,
        "length": 1,
        "name": "MFG_ID",
        "interval": 0
      }
    ]
  })"_json;
  mon.loadInterface(ifaceConfig);
  mon.loadRegisterMap(regmapConfig);
  mon.start();

  // Fake that a tick has elapsed on scan's pollthread.
  mon.scanTick();
  mon.monitorTick();
  std::vector<ModbusDeviceInfo> devs = mon.listDevices();
  EXPECT_EQ(devs.size(), 1);
  EXPECT_EQ(devs[0].deviceAddress, 161);
  EXPECT_EQ(devs[0].mode, ModbusDeviceMode::ACTIVE);

  commandTimeout = true;

  // Monitor for 10 more ticks, each time it is active.
  // but we are expecting command to have failed
  mon.monitorTick();
  devs = mon.listDevices();
  EXPECT_EQ(devs.size(), 1);
  EXPECT_EQ(devs[0].mode, ModbusDeviceMode::ACTIVE);
  mon.monitorTick();
  devs = mon.listDevices();
  EXPECT_EQ(devs.size(), 1);
  EXPECT_EQ(devs[0].mode, ModbusDeviceMode::DORMANT);
  commandTimeout = false;
  EXPECT_CALL(mon, getTime())
      .WillOnce(Return(std::time(nullptr) + 200))
      .WillOnce(Return(std::time(nullptr) + 400));
  // First time, we are only 200s past last time.
  mon.scanTick();
  devs = mon.listDevices();
  EXPECT_EQ(devs.size(), 1);
  EXPECT_EQ(devs[0].mode, ModbusDeviceMode::DORMANT);
  // second time we are 400s past last time, so it should recover.
  mon.scanTick();
  devs = mon.listDevices();
  EXPECT_EQ(devs.size(), 1);
  EXPECT_EQ(devs[0].mode, ModbusDeviceMode::ACTIVE);
}
