// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "UARTDevice.h"
#include <fcntl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <linux/serial.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <array>
#include <cstring>
#include "TempDir.h"

using namespace rackmon;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;

constexpr const char* kTestDevicePath = "/tmp/fakettyUSB0";

// Base mixin to add setDeviceFd/getDeviceFd to device classes
template <typename Base>
class TestableMixin : public Base {
 public:
  using Base::Base;

  void setDeviceFd(int fd) {
    this->deviceFd_ = fd;
  }

  int getDeviceFd() const {
    return this->deviceFd_;
  }
};

// Testable device class for UARTDevice with mocked ioctl
class TestableUARTDevice : public TestableMixin<UARTDevice> {
 public:
  TestableUARTDevice(const std::string& dev, int baud)
      : TestableMixin<UARTDevice>(dev, baud) {}

  MOCK_METHOD(void, ioctl, (unsigned long, void*), (override));

  using UARTDevice::setAttribute;
  using UARTDevice::write;
};

// Testable device class for AspeedRS485Device with mocked ioctl
class TestableAspeedRS485Device : public TestableMixin<AspeedRS485Device> {
 public:
  TestableAspeedRS485Device(const std::string& dev, int baud)
      : TestableMixin<AspeedRS485Device>(dev, baud) {}

  MOCK_METHOD(void, ioctl, (unsigned long, void*), (override));

  using AspeedRS485Device::waitWrite;
};

// Testable device for open() tests - mocks setAttribute
class TestableUARTDeviceForOpen : public UARTDevice {
 public:
  TestableUARTDeviceForOpen(const std::string& dev, int baud)
      : UARTDevice(dev, baud) {}

  MOCK_METHOD(
      void,
      setAttribute,
      (bool readEnable, int baudrate, Parity parity),
      (override));
};

// Testable device for open() tests - mocks setAttribute and ioctl
class TestableAspeedRS485DeviceForOpen : public AspeedRS485Device {
 public:
  TestableAspeedRS485DeviceForOpen(const std::string& dev, int baud)
      : AspeedRS485Device(dev, baud) {}

  MOCK_METHOD(
      void,
      setAttribute,
      (bool readEnable, int baudrate, Parity parity),
      (override));
  MOCK_METHOD(void, ioctl, (unsigned long, void*), (override));
};

// Testable device for write() tests - tracks setAttribute calls and waitWrite
class TestableAspeedRS485DeviceForWrite
    : public TestableMixin<AspeedRS485Device> {
 public:
  TestableAspeedRS485DeviceForWrite(const std::string& dev, int baud)
      : TestableMixin<AspeedRS485Device>(dev, baud) {}

  MOCK_METHOD(void, ioctl, (unsigned long, void*), (override));

  void setAttribute(
      bool /* readEnable */,
      int /* baudrate */,
      Parity /* parity */) override {
    setAttributeCallCount_++;
  }

  void waitWrite() override {
    waitWriteCalled_ = true;
  }

  void write(const uint8_t* buf, size_t len) override {
    setAttributeCallCount_ = 0;
    AspeedRS485Device::write(buf, len);
  }

  int setAttributeCallCount_ = 0;
  bool waitWriteCalled_ = false;
};

// Testable device for LocalEchoUARTDevice tests
class TestableLocalEchoUARTDevice : public TestableMixin<LocalEchoUARTDevice> {
 public:
  TestableLocalEchoUARTDevice(const std::string& dev, int baud)
      : TestableMixin<LocalEchoUARTDevice>(dev, baud) {}

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

// Pipe-based test fixture for I/O tests
class PipeTestFixture : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_EQ(pipe(pipefd_), 0);
  }

  void TearDown() override {
    close(pipefd_[0]);
    close(pipefd_[1]);
  }

  int pipefd_[2]{};
};

// Temp file based test fixture for open() tests
class TempFileTestFixture : public ::testing::Test {
 protected:
  TempDirectory tempDir_;
  std::string devicePath_;

  void SetUp() override {
    devicePath_ = tempDir_.path() + "/ttyUSB0";
    int fd = ::open(devicePath_.c_str(), O_CREAT | O_RDWR, 0644);
    ASSERT_GE(fd, 0);
    ::close(fd);
  }
};

// UARTDevice Tests

using UARTDeviceWriteTest = PipeTestFixture;

TEST_F(UARTDeviceWriteTest, WriteFlushesBuffer) {
  TestableUARTDevice dev(kTestDevicePath, 9600);
  dev.setDeviceFd(pipefd_[1]);

  const uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
  EXPECT_NO_THROW(dev.write(data, sizeof(data)));

  uint8_t readBuf[4];
  ssize_t bytesRead = ::read(pipefd_[0], readBuf, sizeof(readBuf));
  EXPECT_EQ(bytesRead, sizeof(data));
  EXPECT_EQ(memcmp(data, readBuf, sizeof(data)), 0);
}

using UARTDeviceSetAttributeTest = PipeTestFixture;

TEST_F(UARTDeviceSetAttributeTest, SetAttributeThrowsOnInvalidBaudrate) {
  TestableUARTDevice dev(kTestDevicePath, 9600);
  dev.setDeviceFd(pipefd_[1]);

  EXPECT_THROW(dev.setAttribute(true, 12345, Parity::NONE), std::out_of_range);
}

TEST_F(UARTDeviceSetAttributeTest, SetAttributeThrowsOnNonTerminalFd) {
  TestableUARTDevice dev(kTestDevicePath, 9600);
  dev.setDeviceFd(pipefd_[1]);

  // All valid parameter combinations throw on non-terminal FDs
  EXPECT_THROW(dev.setAttribute(true, 9600, Parity::NONE), std::runtime_error);
  EXPECT_THROW(dev.setAttribute(true, 9600, Parity::EVEN), std::runtime_error);
  EXPECT_THROW(dev.setAttribute(true, 9600, Parity::ODD), std::runtime_error);
  EXPECT_THROW(dev.setAttribute(false, 9600, Parity::NONE), std::runtime_error);
}

using UARTDeviceOpenTest = TempFileTestFixture;

TEST_F(UARTDeviceOpenTest, OpenCallsSetAttribute) {
  TestableUARTDeviceForOpen dev(devicePath_, 9600);

  EXPECT_CALL(dev, setAttribute(true, 9600, Parity::EVEN)).Times(1);

  EXPECT_NO_THROW(dev.open());
}

// AspeedRS485Device Tests

using AspeedRS485DeviceOpenTest = TempFileTestFixture;

TEST_F(AspeedRS485DeviceOpenTest, OpenCallsSetAttributeAndSetsRS485Flags) {
  TestableAspeedRS485DeviceForOpen dev(devicePath_, 19200);

  EXPECT_CALL(dev, setAttribute(true, 19200, Parity::EVEN)).Times(1);
  EXPECT_CALL(dev, ioctl(TIOCSRS485, _))
      .WillOnce([](unsigned long /* cmd */, void* data) {
        auto* rs485Conf = static_cast<struct serial_rs485*>(data);
        EXPECT_TRUE(rs485Conf->flags & SER_RS485_ENABLED);
        EXPECT_TRUE(rs485Conf->flags & SER_RS485_RTS_AFTER_SEND);
        EXPECT_TRUE(rs485Conf->flags & SER_RS485_RX_DURING_TX);
      });

  EXPECT_NO_THROW(dev.open());
}

class AspeedRS485DeviceWaitWriteTest : public ::testing::Test {};

TEST_F(AspeedRS485DeviceWaitWriteTest, WaitWritePollsUntilTransmitterEmpty) {
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

TEST_F(AspeedRS485DeviceWaitWriteTest, WaitWriteMaxLoopsReached) {
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

using AspeedRS485DeviceWriteTest = PipeTestFixture;

TEST_F(AspeedRS485DeviceWriteTest, WriteCallsWaitWriteAndSetAttributeTwice) {
  TestableAspeedRS485DeviceForWrite dev(kTestDevicePath, 9600);
  dev.setDeviceFd(pipefd_[1]);

  const uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
  dev.write(data, sizeof(data));

  EXPECT_TRUE(dev.waitWriteCalled_);
  EXPECT_EQ(dev.setAttributeCallCount_, 2);
}

TEST_F(AspeedRS485DeviceWriteTest, WriteWritesDataCorrectly) {
  TestableAspeedRS485DeviceForWrite dev(kTestDevicePath, 9600);
  dev.setDeviceFd(pipefd_[1]);

  const uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
  dev.write(data, sizeof(data));

  uint8_t readBuf[4];
  ssize_t bytesRead = ::read(pipefd_[0], readBuf, sizeof(readBuf));
  EXPECT_EQ(bytesRead, sizeof(data));
  EXPECT_EQ(memcmp(data, readBuf, sizeof(data)), 0);
}

// LocalEchoUARTDevice Tests

using LocalEchoUARTDeviceTest = PipeTestFixture;

TEST_F(LocalEchoUARTDeviceTest, WriteThrowsWhenEchoMismatches) {
  TestableLocalEchoUARTDevice dev(kTestDevicePath, 9600);
  dev.setDeviceFd(pipefd_[0]);

  const uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
  const uint8_t echoData[] = {0x01, 0x02, 0xFF, 0x04};
  ::write(pipefd_[1], echoData, sizeof(echoData));

  EXPECT_THROW(dev.write(data, sizeof(data)), std::runtime_error);
}

TEST_F(LocalEchoUARTDeviceTest, WriteThrowsWhenEchoIncomplete) {
  TestableLocalEchoUARTDevice dev(kTestDevicePath, 9600);
  dev.setDeviceFd(pipefd_[0]);

  const uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
  const uint8_t echoData[] = {0x01, 0x02};
  ::write(pipefd_[1], echoData, sizeof(echoData));

  EXPECT_THROW(dev.write(data, sizeof(data)), std::runtime_error);
}
