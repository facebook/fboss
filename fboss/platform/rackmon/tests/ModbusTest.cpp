// Copyright 2021-present Facebook. All Rights Reserved.
#include "Modbus.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace std;
using namespace testing;
using nlohmann::json;
using namespace rackmon;

class MockUARTDevice : public UARTDevice {
 public:
  MockUARTDevice(const std::string& dev, int baud) : UARTDevice(dev, baud) {}
  MOCK_METHOD0(open, void());
  MOCK_METHOD0(close, void());
  MOCK_METHOD0(exists, bool());
  MOCK_METHOD2(write, void(const uint8_t*, size_t));
  MOCK_METHOD2(ioctl, void(unsigned long, void*));
  MOCK_METHOD1(waitRead, int(int));
  MOCK_METHOD0(waitWrite, void());
  MOCK_METHOD3(read, size_t(uint8_t*, size_t, int));
  MOCK_METHOD2(setAttribute, void(bool, int));
};

class MockModbus : public Modbus {
 public:
  explicit MockModbus(std::stringstream& prof) : Modbus(prof) {}
  virtual ~MockModbus() {
    getHealthCheckThread().stop();
  }
  MOCK_METHOD3(
      makeDevice,
      std::unique_ptr<UARTDevice>(
          const std::string&,
          const std::string&,
          uint32_t));
  void tick() {
    getHealthCheckThread().tick();
  }
};

// Action to set an out-parameter-c-style buffer in an expected call
// to the given buffer and size. See use of SetBufArgNPointeeTo
ACTION_TEMPLATE(
    SetBufArgNPointeeTo,
    HAS_1_TEMPLATE_PARAMS(unsigned, uIndex),
    AND_2_VALUE_PARAMS(pData, uiDataSize)) {
  std::memcpy(std::get<uIndex>(args), pData, uiDataSize);
}

// Matces a buffer with an expected buffer. See use of writeBufferEqual
MATCHER_P2(writeBufferEqual, exp_buf, exp_buf_size, "") {
  for (unsigned int i = 0; i < exp_buf_size; i++) {
    if (arg[i] != exp_buf[i]) {
      std::cout << int(arg[i]) << " != " << int(exp_buf[i]) << std::endl;
      return false;
    }
  }
  return true;
}

class ModbusTest : public ::testing::Test {
 public:
  std::unique_ptr<UARTDevice> make_dev() {
    std::unique_ptr<MockUARTDevice> ptr =
        std::make_unique<MockUARTDevice>("/dev/ttyUSB0", 19200);
    EXPECT_CALL(*ptr, open()).Times(1);
    EXPECT_CALL(*ptr, close()).Times(1);
    EXPECT_CALL(*ptr, exists()).WillRepeatedly(Return(true));
    std::unique_ptr<UARTDevice> ptr2 = std::move(ptr);
    return ptr2;
  }

  std::unique_ptr<UARTDevice> make_flaky_dev() {
    std::unique_ptr<MockUARTDevice> ptr =
        std::make_unique<MockUARTDevice>("/dev/ttyUSB0", 19200);
    EXPECT_CALL(*ptr, open())
        .Times(4)
        .WillOnce(Throw(std::runtime_error("nodev"))) // 1. T0 fail initialize()
        .WillOnce(
            Throw(std::runtime_error("nodev"))) // 2. T0 fail healthThread()
        .WillOnce(Return()) // 3. T1 healthThread() success
        .WillOnce(Return()); // 6. T3 healthThread() success/recover
    EXPECT_CALL(*ptr, close())
        .Times(2); // 5. T2 healthThread(), 8. TX destructor
    EXPECT_CALL(*ptr, exists())
        .Times(2)
        .WillOnce(Return(false)) // 4. T2 vanishes.
        .WillOnce(Return(true)); // 7. T4 all is fine.
    std::unique_ptr<UARTDevice> ptr2 = std::move(ptr);
    return ptr2;
  }

  std::unique_ptr<UARTDevice> make_cmd_dev(
      int baud,
      uint8_t* exp_write,
      size_t exp_write_size,
      uint8_t* read_bytes,
      size_t read_bytes_size) {
    std::unique_ptr<MockUARTDevice> ptr =
        std::make_unique<MockUARTDevice>("/dev/ttyUSB0", 19200);
    EXPECT_CALL(*ptr, open()).Times(1);
    // Expect setAttribute to be called since the baud calling is different.
    if (baud != 19200)
      EXPECT_CALL(*ptr, setAttribute(true, baud)).Times(1);
    // This is tricky one, we want to check if write() was called with our bytes
    // including the CRC tail.
    EXPECT_CALL(
        *ptr,
        write(writeBufferEqual(exp_write, exp_write_size), exp_write_size))
        .Times(1);
    // Another tricky, set the out read_bytes to the one we want to "mock".
    EXPECT_CALL(*ptr, read(_, read_bytes_size, _))
        .Times(1)
        .WillOnce(DoAll(
            SetBufArgNPointeeTo<0>(read_bytes, read_bytes_size),
            Return(read_bytes_size)));
    EXPECT_CALL(*ptr, close()).Times(1);
    EXPECT_CALL(*ptr, exists()).WillRepeatedly(Return(true));

    std::unique_ptr<UARTDevice> ptr2 = std::move(ptr);
    return ptr2;
  }

 protected:
  void SetUp() override {}
};

TEST_F(ModbusTest, BasicDefaultInitialization) {
  nlohmann::json conf;
  conf["device_path"] = "/dev/ttyUSB0";
  conf["baudrate"] = 19200;
  std::stringstream ss;
  MockModbus bus(ss);
  EXPECT_CALL(bus, makeDevice("default", "/dev/ttyUSB0", 19200))
      .Times(1)
      .WillOnce(Return(ByMove(make_dev())));
  bus.initialize(conf);
  EXPECT_EQ(bus.getDefaultBaudrate(), 19200);
  EXPECT_EQ(bus.name(), "/dev/ttyUSB0");
}

TEST_F(ModbusTest, BasicExplicitTypeInitialization) {
  std::vector<std::string> types = {"default", "AspeedRS485"};
  for (const auto& type : types) {
    nlohmann::json conf;
    conf["device_path"] = "/dev/ttyUSB0";
    conf["device_type"] = type;
    conf["baudrate"] = 19200;
    std::stringstream ss;
    MockModbus bus(ss);
    EXPECT_CALL(bus, makeDevice(type, "/dev/ttyUSB0", 19200))
        .Times(1)
        .WillOnce(Return(ByMove(make_dev())));
    bus.initialize(conf);
    EXPECT_EQ(bus.getDefaultBaudrate(), 19200);
    EXPECT_EQ(bus.name(), "/dev/ttyUSB0");
  }
}

TEST_F(ModbusTest, Command) {
  nlohmann::json conf;
  conf["device_path"] = "/dev/ttyUSB0";
  conf["baudrate"] = 19200;

  // Written buffer 0-7 and its CRC16.
  uint8_t write_exp_buf[] = {0, 1, 2, 3, 4, 5, 6, 7, 0x46, 0x7a};
  // mocked response with its CRC.
  uint8_t resp_buf[] = {0, 1, 2, 3, 4, 5, 6, 0xa1, 0xc6};
  std::stringstream ss;
  MockModbus bus(ss);
  EXPECT_CALL(bus, makeDevice("default", "/dev/ttyUSB0", 19200))
      .Times(1)
      .WillOnce(
          Return(ByMove(make_cmd_dev(115200, write_exp_buf, 10, resp_buf, 9))));
  bus.initialize(conf);
  Msg req = 0x0001020304050607_M;
  Msg resp;
  resp.len = 9;
  bus.command(req, resp, 115200, ModbusTime::zero(), ModbusTime::zero());
  Msg exp_resp = 0x00010203040506_M;
  EXPECT_EQ(resp, exp_resp);
}

TEST_F(ModbusTest, CommandBadResp) {
  nlohmann::json conf;
  conf["device_path"] = "/dev/ttyUSB0";
  conf["baudrate"] = 19200;

  // Written buffer 0-7 and its CRC16.
  uint8_t write_exp_buf[] = {0, 1, 2, 3, 4, 5, 6, 7, 0x46, 0x7a};
  // mocked response with its CRC.
  uint8_t resp_buf[] = {
      0, 1, 2, 3, 4, 5, 6, 0xa1, 0xc7}; // 0xa1c6 is correct CRC.
  std::stringstream ss;
  MockModbus bus(ss);
  EXPECT_CALL(bus, makeDevice("default", "/dev/ttyUSB0", 19200))
      .Times(1)
      .WillOnce(
          Return(ByMove(make_cmd_dev(115200, write_exp_buf, 10, resp_buf, 9))));
  bus.initialize(conf);
  Msg req = 0x0001020304050607_M;
  Msg resp;
  resp.len = 9;
  EXPECT_THROW(
      bus.command(req, resp, 115200, ModbusTime::zero(), ModbusTime::zero()),
      CRCError);
}

TEST_F(ModbusTest, TestFlakyIntfRecover) {
  nlohmann::json conf;
  conf["device_path"] = "/dev/ttyUSB0";
  conf["baudrate"] = 19200;
  std::stringstream ss;
  MockModbus bus(ss);
  EXPECT_CALL(bus, makeDevice("default", "/dev/ttyUSB0", 19200))
      .Times(1)
      .WillOnce(Return(ByMove(make_flaky_dev())));
  bus.initialize(conf);
  ASSERT_FALSE(bus.isPresent());
  bus.tick(); // T1
  ASSERT_TRUE(bus.isPresent());
  bus.tick(); // T2
  ASSERT_FALSE(bus.isPresent());
  bus.tick(); // T3
  ASSERT_TRUE(bus.isPresent());
  bus.tick(); // T4
  ASSERT_TRUE(bus.isPresent());
}
