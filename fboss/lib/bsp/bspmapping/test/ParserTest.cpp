// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/bspmapping/Parser.h"
#include <gtest/gtest.h>
#include "fboss/lib/if/gen-cpp2/fboss_common_types.h"

using namespace ::testing;

TEST(ParserTest, GetNameForTests) {
  EXPECT_EQ(
      facebook::fboss::Parser::getNameFor(
          facebook::fboss::PlatformType::PLATFORM_MONTBLANC),
      "montblanc");
  EXPECT_EQ(
      facebook::fboss::Parser::getNameFor(
          facebook::fboss::PlatformType::PLATFORM_MERU400BFU),
      "meru400bfu");
  EXPECT_EQ(
      facebook::fboss::Parser::getNameFor(
          facebook::fboss::PlatformType::PLATFORM_MERU400BIU),
      "meru400biu");
  EXPECT_EQ(
      facebook::fboss::Parser::getNameFor(
          facebook::fboss::PlatformType::PLATFORM_MERU800BIA),
      "meru800bia");
  EXPECT_EQ(
      facebook::fboss::Parser::getNameFor(
          facebook::fboss::PlatformType::PLATFORM_MERU800BFA),
      "meru800bfa");
  EXPECT_EQ(
      facebook::fboss::Parser::getNameFor(
          facebook::fboss::PlatformType::PLATFORM_JANGA800BIC),
      "janga800bic");
  EXPECT_EQ(
      facebook::fboss::Parser::getNameFor(
          facebook::fboss::PlatformType::PLATFORM_TAHAN800BC),
      "tahan800bc");
  EXPECT_EQ(
      facebook::fboss::Parser::getNameFor(
          facebook::fboss::PlatformType::PLATFORM_MORGAN800CC),
      "morgan800cc");
}
