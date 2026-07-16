/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/PlatformMappingUtils.h"

#include <filesystem>

#include <folly/Conv.h>
#include <folly/FileUtil.h>
#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>

#include "fboss/lib/platforms/PlatformDescriptor.h"

namespace fs = std::filesystem;

using namespace facebook::fboss;

namespace {

std::string createDescriptorJson() {
  return R"({
        "platformType": )" +
      folly::to<std::string>(
             static_cast<int>(PlatformType::PLATFORM_WEDGE800BACT)) +
      R"(,
        "productNamePrefixes": ["Wedge800BACT"],
        "modeNames": ["wedge800bact"],
        "asicType": )" +
      folly::to<std::string>(
             static_cast<int>(cfg::AsicType::ASIC_TYPE_TOMAHAWK5)) +
      R"(
      })";
}

std::string createPlatformMappingJson() {
  return R"({
        "ports": {},
        "chips": [],
        "platformSupportedProfiles": []
      })";
}

} // namespace

TEST(TestPlatformMappingUtils, initPlatformMappingUsesDescriptorMapping) {
  auto tmpDir = folly::test::TemporaryDirectory();
  auto platformDir = fs::path(tmpDir.path().string()) / "platforms" / "accton" /
      "wedge800bact";
  fs::create_directories(platformDir);
  ASSERT_TRUE(
      folly::writeFile(
          createDescriptorJson(),
          (platformDir / "platform_descriptor.json").string().c_str()));
  ASSERT_TRUE(
      folly::writeFile(
          createPlatformMappingJson(),
          (platformDir / "platform_mapping.json").string().c_str()));

  FLAGS_platform_mapping_override_path = "";
  FLAGS_platform_descriptor_config_path =
      (fs::path(tmpDir.path().string()) / "platforms").string();

  auto platformMapping =
      utility::initPlatformMapping(PlatformType::PLATFORM_WEDGE800BACT);

  EXPECT_TRUE(platformMapping->getPlatformPorts().empty());
  EXPECT_TRUE(platformMapping->getChips().empty());
}
