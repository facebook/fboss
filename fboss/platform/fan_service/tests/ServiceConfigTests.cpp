/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/fan_service/Bsp.h"
#include "fboss/platform/fan_service/ServiceConfig.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_config_structs_types.h"

#include <gtest/gtest.h>

using namespace facebook::fboss::platform;
using facebook::fboss::FbossError;

class ServiceConfigTest : public ::testing::Test {
  static auto constexpr kSensorName = "sensor";

 public:
  int parseMokujin() {
    int rc = 0;
    ServiceConfig myConfig;
    std::string mokujinConfig = getMokujinFSConfig();
    rc = myConfig.parseConfigString(mokujinConfig);
    return rc;
  }

  int parseInvalidJson() {
    int rc = 0;
    ServiceConfig myConfig;
    // Malformed JSON with missing bracket at the end.
    // The parsing should fail.
    std::string badFormatConfig = "{ \"bsp\" : \"darwin\" ";
    rc = myConfig.parseConfigString(badFormatConfig);
    return rc;
  }
  int parseInvalidConfig() {
    int rc = 0;
    ServiceConfig myConfig;
    // Valid JSON, but the key is invalid. The parsing should fail.
    std::string badKeyConfig = "{ \"bad_key\" : true }";
    rc = myConfig.parseConfigString(badKeyConfig);
    return rc;
  }
};

TEST_F(ServiceConfigTest, parseMokujin) {
  EXPECT_EQ(parseMokujin(), 0);
}

TEST_F(ServiceConfigTest, parseBadJson) {
  EXPECT_THROW(parseInvalidJson(), folly::json::parse_error);
}

TEST_F(ServiceConfigTest, parseInvalidKey) {
  EXPECT_THROW(parseInvalidConfig(), facebook::fboss::FbossError);
}
