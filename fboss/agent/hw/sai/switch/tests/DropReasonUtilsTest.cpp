// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/DropReasonUtils.h"

#include <algorithm>

#include <gtest/gtest.h>

#include <span>

using namespace facebook::fboss;

namespace {
// Made up names rather than real vendor enumerators. These helpers take the
// prefix and suffix as parameters and have no SDK knowledge, so a real name
// exercises exactly the same code as a synthetic one. What matters is the
// shape a stringified X-macro entry has: "<PREFIX><NAME><SUFFIX> = <value>,".
constexpr std::string_view kPrefix = "TEST_DROP_CAUSE_";
constexpr std::string_view kSuffix = "_X";

const char* const kEnums[] = {
    "TEST_DROP_CAUSE_ALPHA_X = 0,",
    "TEST_DROP_CAUSE_BRAVO_X = 1,",
    "TEST_DROP_CAUSE_CHARLIE_X = 2,",
    "TEST_DROP_CAUSE_DELTA_X = 3,",
    "TEST_DROP_CAUSE_ECHO_X = 4,",
};
} // namespace

TEST(DropReasonUtilsTest, extractStripsPrefixSuffixAndValue) {
  EXPECT_EQ(extractDropReasonName(kEnums[0], kPrefix, kSuffix), "ALPHA");
}

TEST(DropReasonUtilsTest, extractStripsTrailingEnumValue) {
  EXPECT_EQ(extractDropReasonName(kEnums[4], kPrefix, kSuffix), "ECHO");
}

TEST(DropReasonUtilsTest, extractHandlesMissingValueTail) {
  EXPECT_EQ(
      extractDropReasonName("TEST_DROP_CAUSE_FOXTROT_X", kPrefix, kSuffix),
      "FOXTROT");
}

TEST(DropReasonUtilsTest, extractHandlesMissingPrefix) {
  EXPECT_EQ(
      extractDropReasonName("OTHER_ENUM_X = 5,", kPrefix, kSuffix),
      "OTHER_ENUM");
}

TEST(DropReasonUtilsTest, extractHandlesMissingSuffix) {
  EXPECT_EQ(
      extractDropReasonName("TEST_DROP_CAUSE_GOLF", kPrefix, kSuffix), "GOLF");
}

TEST(DropReasonUtilsTest, extractHandlesInputShorterThanSuffix) {
  EXPECT_EQ(extractDropReasonName("A", kPrefix, kSuffix), "A");
}

TEST(DropReasonUtilsTest, extractHandlesNull) {
  EXPECT_TRUE(extractDropReasonName(nullptr, kPrefix, kSuffix).empty());
}

// The element type is spelled out throughout: joinDropReasonNames is
// overloaded on std::string and std::string_view, so a braced list of string
// literals is ambiguous.
TEST(DropReasonUtilsTest, joinSeparatesWithComma) {
  EXPECT_EQ(
      joinDropReasonNames(std::vector<std::string>{"ONE", "TWO", "THREE"}),
      "ONE, TWO, THREE");
}

TEST(DropReasonUtilsTest, joinSkipsEmptyNames) {
  EXPECT_EQ(
      joinDropReasonNames(std::vector<std::string>{"ONE", "", "TWO"}),
      "ONE, TWO");
}

TEST(DropReasonUtilsTest, joinOfNothingIsEmpty) {
  EXPECT_TRUE(joinDropReasonNames(std::vector<std::string>{}).empty());
}

TEST(DropReasonUtilsTest, joinAcceptsStringViews) {
  EXPECT_EQ(
      joinDropReasonNames(std::vector<std::string_view>{"ONE", "", "TWO"}),
      "ONE, TWO");
}

TEST(DropReasonUtilsTest, joinTruncatesPastTheCap) {
  // Each name is 50 chars, so the 6th would push past the 256 char cap.
  std::vector<std::string> names(10, std::string(50, 'X'));
  auto joined = joinDropReasonNames(names);

  EXPECT_TRUE(joined.ends_with("<truncated>"));
  // 5 names + 4 separators = 258 > 256, so only 4 names fit before the
  // truncation marker.
  EXPECT_EQ(std::count(joined.begin(), joined.end(), ','), 4);
}

TEST(DropReasonUtilsTest, decodeOfZeroBitmapIsEmpty) {
  EXPECT_TRUE(decodeDropBitmap(0, kEnums, kPrefix, kSuffix).empty());
}

TEST(DropReasonUtilsTest, decodeSingleBit) {
  EXPECT_EQ(decodeDropBitmap(1 << 0, kEnums, kPrefix, kSuffix), "ALPHA");
}

TEST(DropReasonUtilsTest, decodeHighestBitInTable) {
  EXPECT_EQ(decodeDropBitmap(1 << 4, kEnums, kPrefix, kSuffix), "ECHO");
}

TEST(DropReasonUtilsTest, decodeReturnsOnlySetBits) {
  // Bits 0 and 2 set.
  EXPECT_EQ(
      decodeDropBitmap(0b101, kEnums, kPrefix, kSuffix), "ALPHA, CHARLIE");
}

TEST(DropReasonUtilsTest, decodeAllBitsInTable) {
  EXPECT_EQ(
      decodeDropBitmap(0b11111, kEnums, kPrefix, kSuffix),
      "ALPHA, BRAVO, CHARLIE, DELTA, ECHO");
}

TEST(DropReasonUtilsTest, decodeIgnoresBitsPastTheTable) {
  // Bit 5 has no table entry; bit 1 does.
  EXPECT_EQ(decodeDropBitmap(0b100010, kEnums, kPrefix, kSuffix), "BRAVO");
}

TEST(DropReasonUtilsTest, decodeIgnoresAllBitsPastTheTable) {
  // Every one of the 64 bits set, but only the table entries have names: the
  // rest must be dropped rather than read off the end of the table.
  EXPECT_EQ(
      decodeDropBitmap(-1, kEnums, kPrefix, kSuffix),
      decodeDropBitmap(0b11111, kEnums, kPrefix, kSuffix));
}

TEST(DropReasonUtilsTest, decodeSkipsNullTableEntries) {
  const char* const sparse[] = {
      "TEST_DROP_CAUSE_FIRST_X = 0,",
      nullptr,
      "TEST_DROP_CAUSE_THIRD_X = 2,",
  };
  EXPECT_EQ(decodeDropBitmap(0b111, sparse, kPrefix, kSuffix), "FIRST, THIRD");
}

TEST(DropReasonUtilsTest, decodeSkipsNullAtBitZero) {
  const char* const sparse[] = {
      nullptr,
      "TEST_DROP_CAUSE_SECOND_X = 1,",
  };
  EXPECT_EQ(decodeDropBitmap(0b11, sparse, kPrefix, kSuffix), "SECOND");
}

TEST(DropReasonUtilsTest, decodeRejectsBadTable) {
  EXPECT_TRUE(
      decodeDropBitmap(0b1, std::span<const char* const>{}, kPrefix, kSuffix)
          .empty());
}

TEST(DropReasonUtilsTest, decodeTruncatesLongOutput) {
  const char* const longEnums[] = {
      "TEST_DROP_CAUSE_AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA_X = 0,",
      "TEST_DROP_CAUSE_BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB_X = 1,",
      "TEST_DROP_CAUSE_CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC_X = 2,",
      "TEST_DROP_CAUSE_DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD_X = 3,",
      "TEST_DROP_CAUSE_EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE_X = 4,",
      "TEST_DROP_CAUSE_FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF_X = 5,",
      "TEST_DROP_CAUSE_GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG_X = 6,",
      "TEST_DROP_CAUSE_HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH_X = 7,",
      "TEST_DROP_CAUSE_IIIIIIIIIIIIIIIIIIIIIIIIIIIIIII_X = 8,",
      "TEST_DROP_CAUSE_JJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJ_X = 9,",
  };
  auto result = decodeDropBitmap(0x3FF, longEnums, kPrefix, kSuffix);
  EXPECT_TRUE(result.ends_with("<truncated>"));
}
