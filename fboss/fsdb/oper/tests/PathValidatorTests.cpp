// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/fsdb/oper/ExtendedPathBuilder.h"
#include "fboss/fsdb/oper/PathValidator.h"

TEST(PathValidatorTests, ValidateStatePath) {
  using namespace facebook::fboss::fsdb;

  // paths + whether we expect success
  std::vector<std::pair<std::vector<std::string>, bool>> tests = {
      {{"agent"}, true},
      {{"agent", "config"}, true},
      {{"agent", "switchState"}, true},
      {{"agent", "switchStateNonexistent"}, false},
      {{"bgp"}, true},
      {{"qsfp_service"}, true},
      {{"qsfp_service", "state"}, true},
      {{"nonexistent"}, false}};

  for (const auto& [path, successExpected] : tests) {
    if (successExpected) {
      PathValidator::validateStatePath(path);
    } else {
      EXPECT_THROW(PathValidator::validateStatePath(path), FsdbException);
    }
  }
}

TEST(PathValidatorTests, ValidateExtendedStatePath) {
  using namespace facebook::fboss::fsdb;

  // paths + whether we expect success
  std::vector<std::pair<ExtendedOperPath, bool>> tests = {
      {ext_path_builder::raw("agent").raw("switchState").get(), true},
      // regex against non-container
      {ext_path_builder::raw("agent").raw("switchState").regex("port.*").get(),
       false},
      // any against non-container
      {ext_path_builder::raw("agent").raw("switchState").any().get(), false},
      {ext_path_builder::raw("agent").raw("switchState").raw("portMaps").get(),
       true},
      {ext_path_builder::raw("agent")
           .raw("switchState")
           .raw("portMaps")
           .regex("1[0-5]")
           .get(),
       true},
      {ext_path_builder::raw("agent")
           .raw("switchState")
           .raw("portMaps")
           .any()
           .get(),
       true},
      // nonexistent raw path
      {ext_path_builder::raw("agent").raw("switchStateNonexistent").get(),
       false}};

  for (const auto& [path, successExpected] : tests) {
    if (successExpected) {
      PathValidator::validateExtendedStatePath(path);
    } else {
      EXPECT_THROW(
          PathValidator::validateExtendedStatePath(path), FsdbException);
    }
  }
}
