// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include <numeric>

#include <folly/FileUtil.h>
#include <folly/testing/TestUtil.h>

#include "fboss/platform/weutil/Crc16CcittAug.h"
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

// --- calculateCrc16 tests ---

TEST_F(ParserUtilsTest, CalculateCrc16ValidBuffer) {
  // 10-byte buffer: first 6 bytes are the payload for CRC calculation
  // (len - 2 bytes TL - 2 bytes CRC value = 6 payload bytes).
  std::vector<uint8_t> buffer = {
      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0xFE, 0x02, 0xAA, 0xBB};
  uint16_t crc = ParserUtils::calculateCrc16(buffer.data(), buffer.size());
  // Re-derive expected CRC over the same 6-byte payload using the same
  // underlying helper so the test doesn't hardcode implementation output.
  uint16_t expected = helpers::crc_ccitt_aug(buffer.data(), 6);
  EXPECT_EQ(crc, expected);
}

TEST_F(ParserUtilsTest, CalculateCrc16BufferTooSmallThrows) {
  std::vector<uint8_t> tiny = {0x01, 0x02, 0x03, 0x04};
  EXPECT_THROW(
      ParserUtils::calculateCrc16(tiny.data(), tiny.size()),
      std::runtime_error);
}

// --- parseBeUint tests ---

TEST_F(ParserUtilsTest, ParseBeUintSingleByte) {
  unsigned char data[] = {0xAB};
  EXPECT_EQ(ParserUtils::parseBeUint(1, data), std::to_string(0xAB));
}

TEST_F(ParserUtilsTest, ParseBeUintTwoBytes) {
  unsigned char data[] = {0x01, 0x00};
  EXPECT_EQ(ParserUtils::parseBeUint(2, data), std::to_string(256));
}

TEST_F(ParserUtilsTest, ParseBeUintFourBytes) {
  // 0x00, 0x01, 0x00, 0x00 → 65536
  unsigned char data[] = {0x00, 0x01, 0x00, 0x00};
  EXPECT_EQ(ParserUtils::parseBeUint(4, data), std::to_string(65536));
}

TEST_F(ParserUtilsTest, ParseBeUintLenGreaterThan4Throws) {
  unsigned char data[5] = {};
  EXPECT_THROW(ParserUtils::parseBeUint(5, data), std::runtime_error);
}

// --- parseBeHex tests ---

TEST_F(ParserUtilsTest, ParseBeHexSingleByte) {
  unsigned char data[] = {0xFF};
  EXPECT_EQ(ParserUtils::parseBeHex(1, data), "0xff");
}

TEST_F(ParserUtilsTest, ParseBeHexMultiBytes) {
  unsigned char data[] = {0xDE, 0xAD};
  EXPECT_EQ(ParserUtils::parseBeHex(2, data), "0xdead");
}

TEST_F(ParserUtilsTest, ParseBeHexLeadingZeros) {
  unsigned char data[] = {0x0A};
  EXPECT_EQ(ParserUtils::parseBeHex(1, data), "0x0a");
}

// --- parseString tests ---

TEST_F(ParserUtilsTest, ParseStringNormal) {
  unsigned char data[] = {'H', 'e', 'l', 'l', 'o'};
  EXPECT_EQ(ParserUtils::parseString(5, data), "Hello");
}

TEST_F(ParserUtilsTest, ParseStringStopsAtNull) {
  unsigned char data[] = {'A', 'B', 0x00, 'C', 'D'};
  EXPECT_EQ(ParserUtils::parseString(5, data), "AB");
}

TEST_F(ParserUtilsTest, ParseStringEmpty) {
  unsigned char data[] = {0x00, 'X', 'Y'};
  EXPECT_EQ(ParserUtils::parseString(3, data), "");
}

// --- parseMac tests ---

TEST_F(ParserUtilsTest, ParseMacStandard) {
  // 6-byte MAC aa:bb:cc:dd:ee:ff followed by 2-byte count (0x00, 0x01 → "1")
  unsigned char data[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x01};
  EXPECT_EQ(ParserUtils::parseMac(8, data), "aa:bb:cc:dd:ee:ff,1");
}

} // namespace facebook::fboss::platform
