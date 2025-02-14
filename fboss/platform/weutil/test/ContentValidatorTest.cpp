// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/platform/weutil/ContentValidator.h"

using namespace ::testing;

class ContentValidatorTest : public ::testing::Test {
 protected:
  facebook::fboss::platform::ContentValidator validator;
};

TEST_F(ContentValidatorTest, ValidVersion) {
  std::vector<std::pair<std::string, std::string>> contents = {
      {"Version", "4"}};
  EXPECT_TRUE(validator.isValid(contents));
  contents = {{"Version", "5"}};
  EXPECT_TRUE(validator.isValid(contents));
  contents = {{"Version", "6"}, {"Production State", "3"}};
  EXPECT_TRUE(validator.isValid(contents));
}

TEST_F(ContentValidatorTest, InvalidVersion) {
  std::vector<std::pair<std::string, std::string>> contents = {
      {"Version", "3"}};
  EXPECT_FALSE(validator.isValid(contents));
  contents = {{"Version", "7"}};
  EXPECT_FALSE(validator.isValid(contents));
}

TEST_F(ContentValidatorTest, MissingVersion) {
  std::vector<std::pair<std::string, std::string>> contents = {
      {"Production State", "1"}};
  EXPECT_FALSE(validator.isValid(contents));
}

TEST_F(ContentValidatorTest, ValidProductionState) {
  std::vector<std::pair<std::string, std::string>> contents = {
      {"Version", "6"}, {"Production State", "1"}};
  EXPECT_TRUE(validator.isValid(contents));
  contents = {{"Version", "6"}, {"Production State", "2"}};
  EXPECT_TRUE(validator.isValid(contents));
  contents = {{"Version", "6"}, {"Production State", "3"}};
  EXPECT_TRUE(validator.isValid(contents));
  contents = {{"Version", "6"}, {"Production State", "4"}};
  EXPECT_TRUE(validator.isValid(contents));
}

TEST_F(ContentValidatorTest, InvalidProductionState) {
  std::vector<std::pair<std::string, std::string>> contents = {
      {"Version", "6"}, {"Production State", "0"}};
  EXPECT_FALSE(validator.isValid(contents));
  contents = {{"Version", "6"}, {"Production State", "5"}};
  EXPECT_FALSE(validator.isValid(contents));
}

TEST_F(ContentValidatorTest, MissingProductionState) {
  std::vector<std::pair<std::string, std::string>> contents = {
      {"Version", "6"}};
  EXPECT_FALSE(validator.isValid(contents));
}
