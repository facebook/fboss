// Copyright 2021-present Facebook. All Rights Reserved.
#include "ModbusDevice.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>

using namespace std;
using namespace testing;
using nlohmann::json;
using namespace rackmon;

// Mocks the Modbus interface.
class Mock2Modbus : public Modbus {
 public:
  Mock2Modbus() : Modbus() {}
  ~Mock2Modbus() {}
  MOCK_METHOD1(initialize, void(const nlohmann::json&));
  MOCK_METHOD4(command, void(Msg&, Msg&, uint32_t, ModbusTime));
};

// Matches Msg with an expected value.
MATCHER_P(encodeMsgContentEqual, msg_exp, "") {
  Encoder::encode(arg);
  return arg == msg_exp;
}

// Sets an argument of Msg type with the provided literal Msg assumed
// to be already encoded.
ACTION_TEMPLATE(
    SetMsgDecode,
    HAS_1_TEMPLATE_PARAMS(unsigned, uIndex),
    AND_1_VALUE_PARAMS(msg)) {
  std::get<uIndex>(args) = msg;
  Encoder::decode(std::get<uIndex>(args));
}

// Our Test class, sets up the register map and a common device store.
class ModbusDeviceTest : public ::testing::Test {
 protected:
  Mock2Modbus modbus_device;
  RegisterMap regmap;
  std::string regmap_s = R"({
    "name": "orv3_psu",
    "address_range": [110, 140],
    "probe_register": 104,
    "default_baudrate": 19200,
    "preferred_baudrate": 19200,
    "registers": [
      {
        "begin": 0,
        "length": 2,
        "keep": 2,
        "format": "STRING",
        "name": "MFG_MODEL"
      }
    ]
  })";
  void SetUp() override {
    regmap = nlohmann::json::parse(regmap_s);
  }
  Mock2Modbus& get_modbus() {
    return modbus_device;
  }
  RegisterMap& get_regmap() {
    return regmap;
  }
};

// Basic initialization with sane init values as expected
// from the register map and input parameters.
TEST_F(ModbusDeviceTest, BasicSetup) {
  ModbusDevice dev(get_modbus(), 0x32, get_regmap());
  EXPECT_TRUE(dev.isActive());
  ModbusDeviceInfo status = dev.getInfo();
  EXPECT_EQ(status.deviceAddress, 0x32);
  EXPECT_EQ(status.baudrate, 19200);
  EXPECT_EQ(status.crcErrors, 0);
  EXPECT_EQ(status.timeouts, 0);
  EXPECT_EQ(status.miscErrors, 0);
  EXPECT_EQ(status.numConsecutiveFailures, 0);
}

// Basic command interface is a blind pass through.
TEST_F(ModbusDeviceTest, BasicCommand) {
  EXPECT_CALL(get_modbus(), command(Eq(0x3202_M), _, 19200, ModbusTime::zero()))
      .Times(1)
      .WillOnce(SetArgReferee<1>(0x32020304_M));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  Msg req, resp;
  req.raw = {0x32, 2};
  req.len = 2;

  dev.command(req, resp);
  EXPECT_EQ(resp, 0x32020304_M);
}

TEST_F(ModbusDeviceTest, CommandTimeout) {
  EXPECT_CALL(get_modbus(), command(_, _, _, _))
      .Times(3)
      .WillRepeatedly(Throw(TimeoutException()));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap(), 3);

  Msg req, resp;
  EXPECT_THROW(dev.command(req, resp), TimeoutException);
  ModbusDeviceInfo status = dev.getInfo();
  EXPECT_EQ(status.timeouts, 3);
}

TEST_F(ModbusDeviceTest, CommandCRC) {
  EXPECT_CALL(get_modbus(), command(_, _, _, _))
      .Times(5)
      .WillRepeatedly(Throw(CRCError(1, 2)));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  Msg req, resp;
  EXPECT_THROW(dev.command(req, resp), CRCError);
  ModbusDeviceInfo status = dev.getInfo();
  EXPECT_EQ(status.crcErrors, 5);
}

TEST_F(ModbusDeviceTest, CommandMisc) {
  EXPECT_CALL(get_modbus(), command(_, _, _, _))
      .Times(1)
      .WillOnce(Throw(std::runtime_error("")));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap(), 1);

  Msg req, resp;
  EXPECT_THROW(dev.command(req, resp), std::runtime_error);
  ModbusDeviceInfo status = dev.getInfo();
  EXPECT_EQ(status.miscErrors, 1);
}

TEST_F(ModbusDeviceTest, CommandFlaky) {
  EXPECT_CALL(get_modbus(), command(_, _, _, _))
      .Times(2)
      .WillOnce(Invoke([](Msg& req, Msg&, uint32_t, ModbusTime) {
        EXPECT_EQ(req, 0x3202_M);
        Encoder::encode(req);
        throw TimeoutException();
      }))
      .WillOnce(Invoke([](Msg& req, Msg& resp, uint32_t, ModbusTime) {
        EXPECT_EQ(req, 0x3202_M);
        Encoder::encode(req);
        resp = 0x32020304_EM;
        Encoder::decode(resp);
      }));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap(), 3);

  Msg req, resp;
  req.raw = {0x32, 2};
  req.len = 2;
  dev.command(req, resp);
  EXPECT_EQ(resp, 0x32020304_M);
  ModbusDeviceInfo status = dev.getInfo();
  EXPECT_EQ(status.timeouts, 1);
}

TEST_F(ModbusDeviceTest, TimeoutInExclusiveMode) {
  EXPECT_CALL(get_modbus(), command(_, _, _, _))
      .Times(1)
      .WillOnce(Throw(TimeoutException()));
  ModbusDevice dev(get_modbus(), 0x32, get_regmap(), 3);
  dev.setExclusiveMode(true);
  Msg req, resp;
  req.raw = {0x32, 2};
  req.len = 2;
  EXPECT_THROW(dev.command(req, resp), TimeoutException);
  ModbusDeviceInfo status = dev.getInfo();
  EXPECT_EQ(status.timeouts, 1);
}

TEST_F(ModbusDeviceTest, MakeDormant) {
  EXPECT_CALL(get_modbus(), command(_, _, _, _))
      .Times(10)
      .WillRepeatedly(Throw(TimeoutException()));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap(), 1);

  for (int i = 0; i < 10; i++) {
    ModbusDeviceInfo status = dev.getInfo();
    EXPECT_EQ(status.mode, ModbusDeviceMode::ACTIVE);
    Msg req, resp;
    EXPECT_THROW(dev.command(req, resp), TimeoutException);
  }

  ModbusDeviceInfo status = dev.getInfo();
  EXPECT_EQ(status.timeouts, 10);
  EXPECT_EQ(status.mode, ModbusDeviceMode::DORMANT);
}

TEST_F(ModbusDeviceTest, ReadHoldingRegs) {
  EXPECT_CALL(
      get_modbus(),
      command(
          // addr(1) = 0x32,
          // func(1) = 0x03,
          // reg_off(2) = 0x0064,
          // reg_cnt(2) = 0x0002
          encodeMsgContentEqual(0x320300640002_EM),
          _,
          19200,
          ModbusTime::zero()))
      .Times(1)
      // addr(1) = 9x32
      // func(1) = 03
      // bytes(1) = 04
      // data(4) = 11223344
      .WillOnce(SetMsgDecode<1>(0x32030411223344_EM));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  std::vector<uint16_t> regs(2), exp_regs{0x1122, 0x3344};
  dev.readHoldingRegisters(0x64, regs);
  EXPECT_EQ(regs, exp_regs);
}

TEST_F(ModbusDeviceTest, WriteSingleReg) {
  EXPECT_CALL(
      get_modbus(),
      command(
          // addr(1) = 0x32,
          // func(1) = 0x6,
          // reg_off(2) = 0x0064,
          // reg_val(2) = 0x1122
          encodeMsgContentEqual(0x320600641122_EM),
          _,
          19200,
          ModbusTime::zero()))
      .Times(1)
      // addr(1) = 0x32,
      // func(1) = 0x06,
      // reg_off(2) = 0x0064,
      // reg_val(2) = 0x1122
      .WillOnce(SetMsgDecode<1>(0x320600641122_EM));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  std::vector<uint16_t> regs(2), exp_regs{0x1122, 0x3344};
  dev.writeSingleRegister(0x64, 0x1122);
}

TEST_F(ModbusDeviceTest, WriteMultipleReg) {
  EXPECT_CALL(
      get_modbus(),
      command(
          // addr(1) = 0x32,
          // func(1) = 0x10,
          // reg_off(2) = 0x0064,
          // reg_cnt(2) = 0x0002,
          // bytes(1) = 0x04,
          // regs(2*2) = 0x1122 3344
          encodeMsgContentEqual(0x3210006400020411223344_EM),
          _,
          19200,
          ModbusTime::zero()))
      .Times(1)
      // addr(1) = 0x32,
      // func(1) = 0x10,
      // reg_off(2) = 0x0064,
      // reg_cnt(2) = 0x0002
      .WillOnce(SetMsgDecode<1>(0x321000640002_EM));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  std::vector<uint16_t> regs{0x1122, 0x3344};
  dev.writeMultipleRegisters(0x64, regs);
}

TEST_F(ModbusDeviceTest, ReadFileRecord) {
  // Request and response are copied from
  // Page 33, (Adds addr to the head)
  // https://modbus.org/docs/Modbus_Application_Protocol_V1_1b.pdf
  EXPECT_CALL(
      get_modbus(),
      command(
          encodeMsgContentEqual(0x32140E0600040001000206000300090002_EM),
          _,
          19200,
          ModbusTime::zero()))
      .Times(1)
      .WillOnce(SetMsgDecode<1>(0x32140C05060DFE0020050633CD0040_EM));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  std::vector<FileRecord> records(2);
  records[0].data.resize(2);
  records[0].fileNum = 4;
  records[0].recordNum = 1;
  records[1].data.resize(2);
  records[1].fileNum = 3;
  records[1].recordNum = 9;
  dev.readFileRecord(records);
  EXPECT_EQ(records[0].data[0], 0x0DFE);
  EXPECT_EQ(records[0].data[1], 0x20);
  EXPECT_EQ(records[1].data[0], 0x33CD);
  EXPECT_EQ(records[1].data[1], 0x0040);
}

TEST_F(ModbusDeviceTest, DeviceStatus) {
  ModbusDevice dev(get_modbus(), 0x32, get_regmap());
  ModbusDeviceInfo status = dev.getInfo();
  nlohmann::json j = status;
  EXPECT_EQ(status.deviceAddress, 0x32);
  EXPECT_EQ(status.baudrate, 19200);
  EXPECT_EQ(status.crcErrors, 0);
  EXPECT_EQ(status.miscErrors, 0);
  EXPECT_EQ(status.timeouts, 0);
  EXPECT_EQ(status.mode, ModbusDeviceMode::ACTIVE);
  EXPECT_EQ(j["addr"], 0x32);
  EXPECT_EQ(j["crc_fails"], 0);
  EXPECT_EQ(j["misc_fails"], 0);
  EXPECT_EQ(j["timeouts"], 0);
  EXPECT_EQ(j["mode"], "ACTIVE");
  EXPECT_EQ(j["baudrate"], 19200);
}

TEST_F(ModbusDeviceTest, MonitorInvalidRegOnce) {
  EXPECT_CALL(
      get_modbus(),
      command(
          // addr(1) = 0x32,
          // func(1) = 0x03,
          // reg_off(2) = 0x0000,
          // reg_cnt(2) = 0x0002
          encodeMsgContentEqual(0x320300000002_EM),
          _,
          19200,
          ModbusTime::zero()))
      .Times(1)
      // addr(1) = 0x32,
      // func(1) = 0x83,
      // data(1) = 0x02
      .WillOnce(SetMsgDecode<1>(0x328302_EM));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap(), 1);
  // This should see the illegal address error
  dev.reloadRegisters();
  // This should be a no-op.
  dev.reloadRegisters();
}

TEST_F(ModbusDeviceTest, MonitorDataValue) {
  EXPECT_CALL(
      get_modbus(),
      command(
          // addr(1) = 0x32,
          // func(1) = 0x03,
          // reg_off(2) = 0x0000,
          // reg_cnt(2) = 0x0002
          encodeMsgContentEqual(0x320300000002_EM),
          _,
          19200,
          ModbusTime::zero()))
      .Times(3)
      // addr(1) = 0x32,
      // func(1) = 0x03,
      // bytes(1) = 0x04,
      // data(4) = 61626364, 62636465, 63646566
      .WillOnce(SetMsgDecode<1>(0x32030461626364_EM))
      .WillOnce(SetMsgDecode<1>(0x32030462636465_EM))
      .WillOnce(SetMsgDecode<1>(0x32030463646566_EM));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  dev.reloadRegisters();
  ModbusDeviceValueData data = dev.getValueData();
  EXPECT_EQ(data.deviceAddress, 0x32);
  EXPECT_EQ(data.baudrate, 19200);
  EXPECT_EQ(data.crcErrors, 0);
  EXPECT_EQ(data.timeouts, 0);
  EXPECT_EQ(data.miscErrors, 0);
  EXPECT_NEAR(data.lastActive, std::time(0), 10);
  EXPECT_EQ(data.numConsecutiveFailures, 0);
  EXPECT_EQ(data.mode, ModbusDeviceMode::ACTIVE);
  EXPECT_EQ(data.registerList.size(), 1);
  EXPECT_EQ(data.registerList[0].regAddr, 0);
  EXPECT_EQ(data.registerList[0].name, "MFG_MODEL");
  EXPECT_EQ(data.registerList[0].history.size(), 1);
  EXPECT_NEAR(data.registerList[0].history[0].timestamp, std::time(0), 10);
  EXPECT_EQ(data.registerList[0].history[0].type, RegisterValueType::STRING);
  EXPECT_EQ(
      std::get<std::string>(data.registerList[0].history[0].value), "abcd");

  ModbusRegisterFilter filter1, filter2, filter3, filter4;
  filter1.addrFilter = {0x0};
  filter2.addrFilter = {0x10};
  filter3.nameFilter = {"MFG_MODEL"};
  filter4.nameFilter = {"HELLOWORLD"};
  ModbusDeviceValueData filterData1 = dev.getValueData(filter1);
  EXPECT_EQ(filterData1.deviceAddress, 0x32);
  EXPECT_EQ(filterData1.registerList.size(), 1);
  EXPECT_EQ(filterData1.registerList[0].regAddr, 0);
  EXPECT_EQ(filterData1.registerList[0].history.size(), 1);

  ModbusDeviceValueData filterData2 = dev.getValueData(filter2);
  EXPECT_EQ(filterData2.deviceAddress, 0x32);
  EXPECT_EQ(filterData2.registerList.size(), 0);

  ModbusDeviceValueData filterData3 = dev.getValueData(filter3);
  EXPECT_EQ(filterData3.deviceAddress, 0x32);
  EXPECT_EQ(filterData3.registerList.size(), 1);
  EXPECT_EQ(filterData3.registerList[0].regAddr, 0);
  EXPECT_EQ(filterData3.registerList[0].history.size(), 1);

  ModbusDeviceValueData filterData4 = dev.getValueData(filter4);
  EXPECT_EQ(filterData4.deviceAddress, 0x32);
  EXPECT_EQ(filterData4.registerList.size(), 0);

  dev.reloadRegisters();
  ModbusDeviceValueData data2 = dev.getValueData();
  EXPECT_EQ(data2.deviceAddress, 0x32);
  EXPECT_EQ(data2.baudrate, 19200);
  EXPECT_EQ(data2.crcErrors, 0);
  EXPECT_EQ(data2.timeouts, 0);
  EXPECT_EQ(data2.miscErrors, 0);
  EXPECT_NEAR(data2.lastActive, std::time(0), 10);
  EXPECT_EQ(data2.numConsecutiveFailures, 0);
  EXPECT_EQ(data2.mode, ModbusDeviceMode::ACTIVE);
  EXPECT_EQ(data2.registerList.size(), 1);
  EXPECT_EQ(data2.registerList[0].regAddr, 0);
  EXPECT_EQ(data2.registerList[0].name, "MFG_MODEL");
  EXPECT_EQ(data2.registerList[0].history.size(), 2);
  EXPECT_EQ(data2.registerList[0].history[0].type, RegisterValueType::STRING);
  EXPECT_EQ(
      std::get<std::string>(data2.registerList[0].history[0].value), "abcd");
  EXPECT_EQ(data2.registerList[0].history[1].type, RegisterValueType::STRING);
  EXPECT_EQ(
      std::get<std::string>(data2.registerList[0].history[1].value), "bcde");
  EXPECT_NEAR(data2.registerList[0].history[0].timestamp, std::time(0), 10);
  EXPECT_NEAR(data2.registerList[0].history[1].timestamp, std::time(0), 10);
  EXPECT_GE(
      data2.registerList[0].history[1].timestamp,
      data2.registerList[0].history[0].timestamp);

  dev.reloadRegisters();
  ModbusDeviceValueData data3 = dev.getValueData();
  EXPECT_EQ(data3.registerList[0].history.size(), 2);
  // TODO We probably need a circular iterator on the history.
  // Till then, we will probably get out of order stuff.
  EXPECT_EQ(
      std::get<std::string>(data3.registerList[0].history[1].value), "bcde");
  EXPECT_EQ(
      std::get<std::string>(data3.registerList[0].history[0].value), "cdef");
  nlohmann::json j = data3;
  EXPECT_EQ(j["deviceAddress"], 0x32);
  EXPECT_EQ(j["crcErrors"], 0);
  EXPECT_EQ(j["timeouts"], 0);
  EXPECT_EQ(j["miscErrors"], 0);
  EXPECT_EQ(j["mode"], "ACTIVE");
  EXPECT_NEAR(j["now"], std::time(0), 10);
  EXPECT_TRUE(j["registers"].is_array() && j["registers"].size() == 1);
  EXPECT_EQ(j["registers"][0]["regAddress"], 0);
  EXPECT_EQ(j["registers"][0]["name"], "MFG_MODEL");
  EXPECT_TRUE(
      j["registers"][0]["readings"].is_array() &&
      j["registers"][0]["readings"].size() == 2);
  EXPECT_NEAR(j["registers"][0]["readings"][0]["time"], std::time(0), 10);
  EXPECT_EQ(j["registers"][0]["readings"][0]["value"], "cdef");
  EXPECT_EQ(j["registers"][0]["readings"][0]["type"], "STRING");
  EXPECT_NEAR(j["registers"][0]["readings"][1]["time"], std::time(0), 10);
  EXPECT_EQ(j["registers"][0]["readings"][1]["value"], "bcde");
  EXPECT_EQ(j["registers"][0]["readings"][1]["type"], "STRING");

  ModbusDeviceValueData data4 = dev.getValueData({}, true);
  EXPECT_EQ(data4.registerList[0].history.size(), 1);
  EXPECT_EQ(
      std::get<std::string>(data3.registerList[0].history[0].value), "cdef");
}

TEST_F(ModbusDeviceTest, MonitorRawData) {
  EXPECT_CALL(
      get_modbus(),
      command(
          // addr(1) = 0x32,
          // func(1) = 0x03,
          // reg_off(2) = 0x0000,
          // reg_cnt(2) = 0x0002
          encodeMsgContentEqual(0x320300000002_EM),
          _,
          19200,
          ModbusTime::zero()))
      .Times(3)
      // addr(1) = 0x32,
      // func(1) = 0x03,
      // bytes(1) = 0x04,
      // data(4) = 61626364, 62636465, 63646566
      .WillOnce(SetMsgDecode<1>(0x32030461626364_EM))
      .WillOnce(SetMsgDecode<1>(0x32030462636465_EM))
      .WillOnce(SetMsgDecode<1>(0x32030463646566_EM));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  dev.reloadRegisters();
  nlohmann::json data = dev.getRawData();
  EXPECT_EQ(data["addr"], 0x32);
  EXPECT_EQ(data["crc_fails"], 0);
  EXPECT_EQ(data["timeouts"], 0);
  EXPECT_EQ(data["misc_fails"], 0);
  EXPECT_EQ(data["mode"], "ACTIVE");
  EXPECT_NEAR(data["now"], std::time(0), 10);
  EXPECT_TRUE(data["ranges"].is_array() && data["ranges"].size() == 1);
  EXPECT_EQ(data["ranges"][0]["begin"], 0);
  EXPECT_TRUE(
      data["ranges"][0]["readings"].is_array() &&
      data["ranges"][0]["readings"].size() == 1);
  EXPECT_NEAR(data["ranges"][0]["readings"][0]["time"], std::time(0), 10);
  EXPECT_EQ(data["ranges"][0]["readings"][0]["data"], "61626364");

  dev.reloadRegisters();
  nlohmann::json data2 = dev.getRawData();
  EXPECT_EQ(data2["addr"], 0x32);
  EXPECT_EQ(data2["crc_fails"], 0);
  EXPECT_EQ(data2["timeouts"], 0);
  EXPECT_EQ(data2["misc_fails"], 0);
  EXPECT_EQ(data2["mode"], "ACTIVE");
  EXPECT_NEAR(data2["now"], std::time(0), 10);
  EXPECT_TRUE(data2["ranges"].is_array() && data2["ranges"].size() == 1);
  EXPECT_EQ(data2["ranges"][0]["begin"], 0);
  EXPECT_TRUE(
      data2["ranges"][0]["readings"].is_array() &&
      data2["ranges"][0]["readings"].size() == 2);
  EXPECT_NEAR(data2["ranges"][0]["readings"][0]["time"], std::time(0), 10);
  EXPECT_EQ(data2["ranges"][0]["readings"][0]["data"], "61626364");
  EXPECT_NEAR(data2["ranges"][0]["readings"][1]["time"], std::time(0), 10);
  EXPECT_EQ(data2["ranges"][0]["readings"][1]["data"], "62636465");

  dev.reloadRegisters();
  nlohmann::json data3 = dev.getRawData();
  EXPECT_TRUE(
      data3["ranges"][0]["readings"].is_array() &&
      data3["ranges"][0]["readings"].size() == 2);
  EXPECT_NEAR(data3["ranges"][0]["readings"][0]["time"], std::time(0), 10);
  EXPECT_EQ(data3["ranges"][0]["readings"][0]["data"], "63646566");
  EXPECT_NEAR(data3["ranges"][0]["readings"][1]["time"], std::time(0), 10);
  EXPECT_EQ(data3["ranges"][0]["readings"][1]["data"], "62636465");
}

class MockModbusDevice : public ModbusDevice {
 public:
  MockModbusDevice(Modbus& m, uint8_t addr, const RegisterMap& rmap)
      : ModbusDevice(m, addr, rmap) {}
  MOCK_METHOD3(command, void(Msg&, Msg&, ModbusTime));
};

class MockSpecialHandler : public ModbusSpecialHandler {
  time_t currTime_ = 1024;

 public:
  explicit MockSpecialHandler(uint8_t deviceAddress)
      : ModbusSpecialHandler(deviceAddress) {}
  time_t getTime() override {
    return currTime_;
  }
  void incrementTimeBy(time_t incTime) {
    currTime_ += incTime;
  }
};

TEST(ModbusSpecialHandler, BasicHandlingStringValuePeriodic) {
  Modbus mock_modbus{};
  RegisterMap mock_rmap = R"({
    "name": "orv3_psu",
    "address_range": [110, 140],
    "probe_register": 104,
    "default_baudrate": 19200,
    "preferred_baudrate": 19200,
    "registers": [
      {
        "begin": 0,
        "length": 2,
        "name": "MFG_MODEL"
      }
    ]
  })"_json;
  MockModbusDevice dev(mock_modbus, 0x32, mock_rmap);

  EXPECT_CALL(
      dev,
      command(
          // addr(1) = 0x32,
          // func(1) = 0x10,
          // reg_off(2) = 0x000a (10),
          // reg_cnt(2) = 0x0002
          // bytes(1) = 0x04,
          // data(2*2) = 0x3031 0x3233
          encodeMsgContentEqual(0x3210000a00020430313233_EM),
          _,
          _))
      .Times(Between(2, 3));
  MockSpecialHandler special(0x32);
  SpecialHandlerInfo& info = special;
  info = R"({
    "reg": 10,
    "len": 2,
    "period": 10,
    "action": "write",
    "info": {
      "interpret": "STRING",
      "value": "0123"
    }
  })"_json;

  special.handle(dev);
  // Fake advance time by 2s
  special.incrementTimeBy(2);
  special.handle(
      dev); // Since the period is 10, this should technically do nothing.
  // Fake advance time by another 4s.
  special.incrementTimeBy(4);
  special.handle(
      dev); // This should do nothing as well, we are less than 10 sec.
  // Fake advance by 5s.
  special.incrementTimeBy(5);
  special.handle(dev); // This should call! we are 11s out from first handle.
}

TEST(ModbusSpecialHandler, BasicHandlingIntegerOneShot) {
  Modbus mock_modbus{};
  RegisterMap mock_rmap = R"({
    "name": "orv3_psu",
    "address_range": [110, 140],
    "probe_register": 104,
    "default_baudrate": 19200,
    "preferred_baudrate": 19200,
    "registers": [
      {
        "begin": 0,
        "length": 2,
        "name": "MFG_MODEL"
      }
    ]
  })"_json;
  MockModbusDevice dev(mock_modbus, 0x32, mock_rmap);

  EXPECT_CALL(
      dev,
      command(
          // addr(1) = 0x32,
          // func(1) = 0x10,
          // reg_off(2) = 0x000a (10),
          // reg_cnt(2) = 0x0002
          // bytes(1) = 0x04,
          // data(2*2) = 0x00bc 0x614e (hex for int 12345678)
          encodeMsgContentEqual(0x3210000a00020400bc614e_EM),
          _,
          _))
      .Times(1);
  MockSpecialHandler special(0x32);
  SpecialHandlerInfo& info = special;
  info = R"({
    "reg": 10,
    "len": 2,
    "period": -1,
    "action": "write",
    "info": {
      "interpret": "INTEGER",
      "shell": "echo 12345678"
    }
  })"_json;
  // 12345678 == 0x00bc614e

  special.handle(dev);
  special.incrementTimeBy(1);
  special.handle(dev); // Do the same as above, but the call should happen only
                       // once since period = -1
  // fake advance clocks by 10 and 20 seconds and at each time we should
  // not incur any further handling.
  special.incrementTimeBy(10);
  special.handle(dev);
  special.incrementTimeBy(20);
  special.handle(dev);
}

static nlohmann::json getBaudrateRegmap() {
  std::string regmap_s = R"({
    "name": "orv3_psu",
    "address_range": [5, 7],
    "probe_register": 104,
    "default_baudrate": 19200,
    "preferred_baudrate": 115200,
    "baud_config": {
      "reg": 16,
      "baud_value_map": [
        [19200, 1],
        [57600, 2],
        [115200, 256]
      ]
    },
    "registers": [
      {
        "begin": 0,
        "length": 2,
        "keep": 2,
        "format": "STRING",
        "name": "MFG_MODEL"
      }
    ]
  })";
  return nlohmann::json::parse(regmap_s);
}

TEST(ModbusDeviceBaudrate, BaudrateNegotiationTest) {
  RegisterMap regmap = getBaudrateRegmap();
  Mock2Modbus mockdev;
  InSequence seq;
  // Expect baudrate to be set to preferred 115200. Command
  // itself sent at the default 19200 baudrate
  EXPECT_CALL(
      mockdev,
      command(
          // addr(1) = 0x05
          // func(1) = 0x06
          // reg_off(2) = 0x0010 (16),
          // reg_val(2) = 0x0100 (256)
          encodeMsgContentEqual(0x050600100100_EM),
          _,
          19200,
          ModbusTime::zero()))
      .Times(1)
      .WillOnce(SetMsgDecode<1>(0x050600100100_EM));

  // Expect request to read register at new baudrate (115200)
  EXPECT_CALL(
      mockdev,
      command(
          // addr(1) = 0x05
          // func(1) = 0x03
          // reg_off(2) = 0x0000 (0),
          // reg_val(2) = 0x0002 (2)
          encodeMsgContentEqual(0x050300000002_EM),
          _,
          115200,
          ModbusTime::zero()))
      .Times(1)
      .WillOnce(SetMsgDecode<1>(0x05030461626364_EM));

  // Expect on destruction for the baudrate to be reset to
  // the default 19200
  EXPECT_CALL(
      mockdev,
      command(
          // addr(1) = 0x05
          // func(1) = 0x06
          // reg_off(2) = 0x0010 (16),
          // reg_val(2) = 0x0001 (1)
          encodeMsgContentEqual(0x050600100001_EM),
          _,
          115200,
          ModbusTime::zero()))
      .Times(1)
      .WillOnce(SetMsgDecode<1>(0x050600100001_EM));
  {
    ModbusDevice dev(mockdev, 5, regmap, 1);
    dev.reloadRegisters();
    ModbusDeviceValueData data = dev.getValueData();
    EXPECT_EQ(data.deviceAddress, 0x05);
    EXPECT_EQ(data.baudrate, 115200);
    EXPECT_EQ(data.registerList.size(), 1);
    EXPECT_EQ(data.registerList[0].regAddr, 0);
    EXPECT_EQ(data.registerList[0].name, "MFG_MODEL");
    EXPECT_EQ(
        std::get<std::string>(data.registerList[0].history[0].value), "abcd");
  }
}

TEST(ModbusDeviceBaudrate, BaudrateNegotiationRejection) {
  RegisterMap regmap = getBaudrateRegmap();
  Mock2Modbus mockdev;
  InSequence seq;
  // Expect the call to set baudrate at 115200. Fake a ModbusError
  // exception thrown. This should cause us to stop negotiation
  // and stick to default.
  EXPECT_CALL(
      mockdev,
      command(
          // addr(1) = 0x05
          // func(1) = 0x06
          // reg_off(2) = 0x0010 (16),
          // reg_val(2) = 0x0100 (256)
          encodeMsgContentEqual(0x050600100100_EM),
          _,
          19200,
          ModbusTime::zero()))
      .Times(1)
      .WillOnce(Throw(ModbusError(3)));

  // Expect request to read register at default baudrate (19200)
  EXPECT_CALL(
      mockdev,
      command(
          // addr(1) = 0x05
          // func(1) = 0x03
          // reg_off(2) = 0x0000 (0),
          // reg_val(2) = 0x0002 (2)
          encodeMsgContentEqual(0x050300000002_EM),
          _,
          19200,
          ModbusTime::zero()))
      .Times(1)
      .WillOnce(SetMsgDecode<1>(0x05030461626364_EM));
  {
    ModbusDevice dev(mockdev, 5, regmap, 1);
    dev.reloadRegisters();
    ModbusDeviceValueData data = dev.getValueData();
    EXPECT_EQ(data.deviceAddress, 0x05);
    EXPECT_EQ(data.baudrate, 19200);
    EXPECT_EQ(data.registerList.size(), 1);
    EXPECT_EQ(data.registerList[0].regAddr, 0);
    EXPECT_EQ(data.registerList[0].name, "MFG_MODEL");
    EXPECT_EQ(
        std::get<std::string>(data.registerList[0].history[0].value), "abcd");
  }
}
