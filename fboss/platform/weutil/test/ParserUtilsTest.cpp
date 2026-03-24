// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include <numeric>

#include <folly/FileUtil.h>
#include <folly/testing/TestUtil.h>

#include "fboss/platform/weutil/ParserUtils.h"

namespace facebook::fboss::platform {

class ParserUtilsTest : public ::testing::Test {
 protected:
  std::string writeTempFile(const std::vector<uint8_t>& data) {
    std::string path = tmpDir_.path().string() + "/eeprom";
    folly::writeFile(data, path.c_str());
    return path;
  }

 private:
  folly::test::TemporaryDirectory tmpDir_;
};

// Verify loadEeprom() reads at most 512 bytes regardless of file size.
TEST_F(ParserUtilsTest, LargerThan512BytesReturns512Bytes) {
  std::vector<uint8_t> data(600, 0xAB);
  auto path = writeTempFile(data);
  auto result = ParserUtils::loadEeprom(path, 0);
  EXPECT_EQ(result.size(), kMaxEepromDataRegionSize);
  EXPECT_EQ(result, std::vector<uint8_t>(kMaxEepromDataRegionSize, 0xAB));
}

// Verify loadEeprom() returns only the actual bytes when file is smaller
// than 512 bytes.
TEST_F(ParserUtilsTest, SmallerThan512BytesReturnsActualSize) {
  std::vector<uint8_t> data(128, 0xCD);
  auto path = writeTempFile(data);
  auto result = ParserUtils::loadEeprom(path, 0);
  EXPECT_EQ(result.size(), 128);
  EXPECT_EQ(result, std::vector<uint8_t>(128, 0xCD));
}

// Verify loadEeprom() throws when the file does not exist.
TEST_F(ParserUtilsTest, NonexistentFileThrows) {
  EXPECT_THROW(
      ParserUtils::loadEeprom("/tmp/nonexistent_eeprom_file", 0),
      std::runtime_error);
}

// Verify loadEeprom() throws when offset is beyond the file size.
TEST_F(ParserUtilsTest, OffsetBeyondFileSizeThrows) {
  std::vector<uint8_t> data(64, 0xEE);
  auto path = writeTempFile(data);
  EXPECT_THROW(ParserUtils::loadEeprom(path, 1024), std::runtime_error);
}

// Verify loadEeprom() returns correct data when using a non-zero offset.
TEST_F(ParserUtilsTest, NonZeroOffsetReadsFromOffset) {
  std::vector<uint8_t> data(256);
  std::iota(data.begin(), data.end(), 0);
  auto path = writeTempFile(data);
  auto result = ParserUtils::loadEeprom(path, 64);
  EXPECT_EQ(result.size(), 192);
  std::vector<uint8_t> expected(192);
  std::iota(expected.begin(), expected.end(), 64);
  EXPECT_EQ(result, expected);
}

} // namespace facebook::fboss::platform
