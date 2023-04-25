// Copyright 2004-present Facebook. All Rights Reserved.

#include <folly/FileUtil.h>
#include <folly/Format.h>
#include <folly/experimental/TestUtil.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
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
    LedMapping ledMapping;
    ledMapping.id() = 0;
    ledMapping.bluePath() = bluePath_;
    ledMapping.yellowPath() = yellowPath_;
    // Instantiating Led object will write 0 to both blue and yellow LED
    // files and set current color to Off
    led_ = std::make_unique<LedIO>(ledMapping);
    blueFd_ = open(bluePath_.c_str(), O_RDWR);
    EXPECT_GE(blueFd_, 0);
    yellowFd_ = open(yellowPath_.c_str(), O_RDWR);
    EXPECT_GE(yellowFd_, 0);
    VerifyLedOff();

    // Since current color is Off, setting it to Off again will be noop
    led_->setColor(led::LedColor::OFF);
    VerifyLedOff();

    // Change current color from Off to Blue
    led_->setColor(led::LedColor::BLUE);
    VerifyBlueOn();

    // Since current color is Blue, setting it to Blue again will be noop
    led_->setColor(led::LedColor::BLUE);
    VerifyBlueOn();

    // Change current color from Blue to Off
    led_->setColor(led::LedColor::OFF);
    VerifyLedOff();

    // Change current color from Off to Yellow
    led_->setColor(led::LedColor::YELLOW);
    VerifyYellowOn();

    // Since current color is Yellow, setting it to Yellow again will be noop
    led_->setColor(led::LedColor::YELLOW);
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
    EXPECT_EQ(led::LedColor::OFF, led_->getColor());
  }

  // Verify that the blue file contains a single 1, the yellow file
  // contains a single 0, and the current color is Blue
  void VerifyBlueOn() {
    EXPECT_EQ('1', ReadLedFile(blueFd_));
    EXPECT_EQ('0', ReadLedFile(yellowFd_));
    EXPECT_EQ(led::LedColor::BLUE, led_->getColor());
  }

  // Verify that the yellow file contains a single 1, the blue file
  // contains a single 0, and the current color is Yellow
  void VerifyYellowOn() {
    EXPECT_EQ('0', ReadLedFile(blueFd_));
    EXPECT_EQ('1', ReadLedFile(yellowFd_));
    EXPECT_EQ(led::LedColor::YELLOW, led_->getColor());
  }

  folly::test::TemporaryDirectory tmpDir_;
  std::string bluePath_ = tmpDir_.path().string() + "/invalidBlueLed";
  std::string yellowPath_ = tmpDir_.path().string() + "/invalidYellowLed";
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
