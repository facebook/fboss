// Copyright 2004-present Facebook. All Rights Reserved.

#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>
#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>
#include "fboss/lib/i2c/I2cDevImpl.h"
#include "fboss/lib/i2c/I2cDevIo.h"

namespace facebook::fboss {

class I2cDevIoTests : public ::testing::Test {
 public:
  /* Verify the ctor I2cDevIo() throws I2cDevIoError exception for non-existent
   * file
   */
  void testInvalidFile() {
    EXPECT_THROW(
        std::unique_ptr<I2cDevIo> i2cDev =
            std::make_unique<I2cDevIo>("/invalidFile/tmp"),
        I2cDevIoError);
  }

  /* Verify the I2cDevIo::read() method throws I2cDevIoError exception if it
   * cannot read from a regular non-i2cDev file
   */
  void testInvalidRead() {
    std::unique_ptr<I2cDevIo> i2cDev =
        std::make_unique<I2cDevIo>(filePath_, I2cIoType::I2cIoTypeForTest);
    uint8_t addr = 0x50;
    uint8_t offset = 127;
    uint8_t buf;
    EXPECT_THROW(i2cDev->read(addr, offset, &buf, 1), I2cDevIoError);
  }

  /* Verify the I2cDevIo::write() method throws I2cDevIoError exception if it
   * cannot write to a regular non-i2cDev file
   */
  void testInvalidWrite() {
    std::unique_ptr<I2cDevIo> i2cDev =
        std::make_unique<I2cDevIo>(filePath_, I2cIoType::I2cIoTypeForTest);
    uint8_t addr = 0x50;
    uint8_t offset = 127;
    const uint8_t buf = 1;
    EXPECT_THROW(i2cDev->write(addr, offset, &buf, 1), I2cDevIoError);
  }

 protected:
  void SetUp() override {
    std::string content = "123456789";
    folly::writeFile(content, filePath_.c_str());
  }

 private:
  folly::test::TemporaryDirectory tmpDir_;
  std::string filePath_ = tmpDir_.path().string() + "/invalidI2cDev";
};

TEST_F(I2cDevIoTests, TestInvalidFile) {
  testInvalidFile();
}

TEST_F(I2cDevIoTests, TestInvalidRead) {
  testInvalidRead();
}

TEST_F(I2cDevIoTests, TestInvalidWrite) {
  testInvalidWrite();
}

} // namespace facebook::fboss
