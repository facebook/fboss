// Copyright 2004-present Facebook. All Rights Reserved.

#include <folly/FileUtil.h>
#include <folly/Format.h>
#include <folly/experimental/TestUtil.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include "fboss/led_service/LedUtils.h"
#include "fboss/lib/led/LedIO.h"

namespace facebook::fboss {

class LedTests : public ::testing::Test {
 public:
  void testInvalidLedPath() {
    LedMapping ledMapping;
    ledMapping.id() = 0;
    EXPECT_THROW(
        std::unique_ptr<LedIO> led = std::make_unique<LedIO>(ledMapping),
        LedIOError);
  }

  void testFakeLed() {
    mkdir(blueBasePath_.c_str(), 0777);
    blueFd_ = open(bluePath_.c_str(), O_RDWR | O_CREAT, 0777);
    EXPECT_GE(blueFd_, 0);

    mkdir(yellowBasePath_.c_str(), 0777);
    yellowFd_ = open(yellowPath_.c_str(), O_RDWR | O_CREAT, 0777);
    EXPECT_GE(yellowFd_, 0);

    LedMapping ledMapping;
    ledMapping.id() = 0;
    ledMapping.bluePath() = blueBasePath_;
    ledMapping.yellowPath() = yellowBasePath_;
    // Instantiating Led object will write 0 to both blue and yellow LED
    // files and set current color to Off
    led_ = std::make_unique<LedIO>(ledMapping);

    VerifyLedOff();

    // Since current color is Off, setting it to Off again will be noop
    led_->setLedState(
        utility::constructLedState(led::LedColor::OFF, led::Blink::OFF));
    VerifyLedOff();

    // Change current color from Off to Blue
    led_->setLedState(
        utility::constructLedState(led::LedColor::BLUE, led::Blink::OFF));
    VerifyBlueOn();

    // Since current color is Blue, setting it to Blue again will be noop
    led_->setLedState(
        utility::constructLedState(led::LedColor::BLUE, led::Blink::OFF));
    VerifyBlueOn();

    // Change current color from Blue to Off
    led_->setLedState(
        utility::constructLedState(led::LedColor::OFF, led::Blink::OFF));
    VerifyLedOff();

    // Change current color from Off to Yellow
    led_->setLedState(
        utility::constructLedState(led::LedColor::YELLOW, led::Blink::OFF));
    VerifyYellowOn();

    // Since current color is Yellow, setting it to Yellow again will be noop
    led_->setLedState(
        utility::constructLedState(led::LedColor::YELLOW, led::Blink::OFF));
    VerifyYellowOn();

    close(blueFd_);
    close(yellowFd_);
  }

 protected:
  void SetUp() override {
    // Write 1 to both blue and yellow LED files initially
    const std::string content = "1";
    folly::writeFile(content, bluePath_.c_str());
    folly::writeFile(content, yellowPath_.c_str());
  }

 private:
  char ReadLedFile(int fd) {
    constexpr auto len = 1;
    char readBuf;

    // Reset to the beginning of the file before reading it
    int retVal = ::lseek(fd, 0, SEEK_SET);
    EXPECT_EQ(retVal, 0);
    retVal = ::read(fd, &readBuf, len);
    EXPECT_EQ(retVal, len);

    return readBuf;
  }

  // Verify that both the blue and yellow LED files contain a single 0 and the
  // current color is Off
  void VerifyLedOff() {
    EXPECT_EQ('0', ReadLedFile(blueFd_));
    EXPECT_EQ('0', ReadLedFile(yellowFd_));
    EXPECT_EQ(led::LedColor::OFF, led_->getLedState().get_ledColor());
  }

  // Verify that the blue file contains a single 1, the yellow file
  // contains a single 0, and the current color is Blue
  void VerifyBlueOn() {
    EXPECT_EQ('1', ReadLedFile(blueFd_));
    EXPECT_EQ('0', ReadLedFile(yellowFd_));
    EXPECT_EQ(led::LedColor::BLUE, led_->getLedState().get_ledColor());
  }

  // Verify that the yellow file contains a single 1, the blue file
  // contains a single 0, and the current color is Yellow
  void VerifyYellowOn() {
    EXPECT_EQ('0', ReadLedFile(blueFd_));
    EXPECT_EQ('1', ReadLedFile(yellowFd_));
    EXPECT_EQ(led::LedColor::YELLOW, led_->getLedState().get_ledColor());
  }

  folly::test::TemporaryDirectory tmpDir_;
  std::string blueBasePath_ = tmpDir_.path().string() + "/invalidBlueLed";
  std::string yellowBasePath_ = tmpDir_.path().string() + "/invalidYellowLed";
  std::string bluePath_ = blueBasePath_ + "/brightness";
  std::string yellowPath_ = yellowBasePath_ + "/brightness";
  int blueFd_;
  int yellowFd_;
  std::unique_ptr<LedIO> led_;
};

TEST_F(LedTests, TestInvalidLedPath) {
  testInvalidLedPath();
}

TEST_F(LedTests, testFakeLed) {
  testFakeLed();
}

} // namespace facebook::fboss
