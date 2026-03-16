// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

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

} // namespace facebook::fboss::platform
