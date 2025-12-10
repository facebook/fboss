// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "UARTDevice.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <linux/serial.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <array>
#include <cstring>

using namespace rackmon;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArrayArgument;
using ::testing::Throw;

constexpr const char* kTestDevicePath = "/tmp/fakettyUSB0";

// Testable device classes that avoid accessing actual hardware by:
// 1. Not calling open() (deviceFd_ remains -1 in constructors)
// 2. Overriding write() to bypass tcflush() which requires terminal FDs
// 3. Using pipes as mock file descriptors for I/O testing
class TestableUARTDevice : public UARTDevice {
 public:
  TestableUARTDevice(const std::string& dev, int baud)
      : UARTDevice(dev, baud) {}

  MOCK_METHOD(void, ioctl, (unsigned long, void*), (override));

  void setDeviceFd(int fd) {
    deviceFd_ = fd;
  }

  int getDeviceFd() const {
    return deviceFd_;
  }

  // Override write to avoid tcflush on non-terminal FDs
  void write(const uint8_t* buf, size_t len) override {
    Device::write(buf, len);
  }

  using UARTDevice::setAttribute;
};

class TestableAspeedRS485Device : public AspeedRS485Device {
 public:
  TestableAspeedRS485Device(const std::string& dev, int baud)
      : AspeedRS485Device(dev, baud) {}

  MOCK_METHOD(void, ioctl, (unsigned long, void*), (override));

  void setDeviceFd(int fd) {
    deviceFd_ = fd;
  }

  int getDeviceFd() const {
    return deviceFd_;
  }

  using AspeedRS485Device::waitWrite;
};

class TestableLocalEchoUARTDevice : public LocalEchoUARTDevice {
 public:
  TestableLocalEchoUARTDevice(const std::string& dev, int baud)
      : LocalEchoUARTDevice(dev, baud) {}

  void setDeviceFd(int fd) {
    deviceFd_ = fd;
  }

  int getDeviceFd() const {
    return deviceFd_;
  }

  // Override write to avoid tcflush on non-terminal FDs
  void write(const uint8_t* buf, size_t len) override {
    const int txTimeoutMs = 100;
    Device::write(buf, len);
    std::array<uint8_t, 256> txCheckBuf{};
    size_t readBytes = read(txCheckBuf.data(), len, txTimeoutMs);
    if (readBytes != len) {
      throw std::runtime_error("Wrote lesser bytes than requested");
    }
    if (memcmp(buf, txCheckBuf.data(), len) != 0) {
      throw std::runtime_error("Mismatch in written buffer");
    }
  }
};

class UARTDeviceTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(UARTDeviceTest, WriteFlushesBuffer) {
  int pipefd[2];
  ASSERT_EQ(pipe(pipefd), 0);

  TestableUARTDevice dev(kTestDevicePath, 9600);
  dev.setDeviceFd(pipefd[1]);

  const uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
  EXPECT_NO_THROW(dev.write(data, sizeof(data)));

  uint8_t readBuf[4];
  ssize_t bytesRead = ::read(pipefd[0], readBuf, sizeof(readBuf));
  EXPECT_EQ(bytesRead, sizeof(data));
  EXPECT_EQ(memcmp(data, readBuf, sizeof(data)), 0);

  close(pipefd[0]);
  close(pipefd[1]);
}

class AspeedRS485DeviceTest : public ::testing::Test {};

TEST_F(AspeedRS485DeviceTest, OpenSetsRS485Flags) {
  struct serial_rs485 rs485Conf{};
  rs485Conf.flags = SER_RS485_ENABLED;
  rs485Conf.flags |= (SER_RS485_RTS_AFTER_SEND | SER_RS485_RX_DURING_TX);

  EXPECT_TRUE(rs485Conf.flags & SER_RS485_ENABLED);
  EXPECT_TRUE(rs485Conf.flags & SER_RS485_RTS_AFTER_SEND);
  EXPECT_TRUE(rs485Conf.flags & SER_RS485_RX_DURING_TX);
}

TEST_F(AspeedRS485DeviceTest, WaitWritePollsUntilTransmitterEmpty) {
  TestableAspeedRS485Device dev(kTestDevicePath, 9600);
  int callCount = 0;

  EXPECT_CALL(dev, ioctl(TIOCSERGETLSR, _))
      .WillOnce(DoAll(
          [&callCount](unsigned long /* cmd */, void* data) {
            callCount++;
            int* lsr = static_cast<int*>(data);
            *lsr = 0;
          },
          Return()))
      .WillOnce(DoAll(
          [&callCount](unsigned long /* cmd */, void* data) {
            callCount++;
            int* lsr = static_cast<int*>(data);
            *lsr = TIOCSER_TEMT;
          },
          Return()));

  dev.waitWrite();
  EXPECT_EQ(callCount, 2);
}

TEST_F(AspeedRS485DeviceTest, WaitWriteMaxLoopsReached) {
  TestableAspeedRS485Device dev(kTestDevicePath, 9600);

  EXPECT_CALL(dev, ioctl(TIOCSERGETLSR, _))
      .WillRepeatedly(DoAll(
          [](unsigned long /* cmd */, void* data) {
            int* lsr = static_cast<int*>(data);
            *lsr = 0;
          },
          Return()));

  dev.waitWrite();
}

class LocalEchoUARTDeviceTest : public ::testing::Test {};

TEST_F(LocalEchoUARTDeviceTest, WriteThrowsWhenEchoMismatches) {
  int pipefd[2];
  ASSERT_EQ(pipe(pipefd), 0);

  TestableLocalEchoUARTDevice dev(kTestDevicePath, 9600);
  dev.setDeviceFd(pipefd[0]);

  const uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
  const uint8_t echoData[] = {0x01, 0x02, 0xFF, 0x04};
  ::write(pipefd[1], echoData, sizeof(echoData));

  EXPECT_THROW(dev.write(data, sizeof(data)), std::runtime_error);

  close(pipefd[0]);
  close(pipefd[1]);
}

TEST_F(LocalEchoUARTDeviceTest, WriteThrowsWhenEchoIncomplete) {
  int pipefd[2];
  ASSERT_EQ(pipe(pipefd), 0);

  TestableLocalEchoUARTDevice dev(kTestDevicePath, 9600);
  dev.setDeviceFd(pipefd[0]);

  const uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
  const uint8_t echoData[] = {0x01, 0x02};
  ::write(pipefd[1], echoData, sizeof(echoData));

  EXPECT_THROW(dev.write(data, sizeof(data)), std::runtime_error);

  close(pipefd[0]);
  close(pipefd[1]);
}
