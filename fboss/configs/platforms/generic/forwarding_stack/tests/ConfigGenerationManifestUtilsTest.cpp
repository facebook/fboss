/*
 *  Copyright (c) Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <exception>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "fboss/configs/platforms/generic/forwarding_stack/utils/ConfigGenerationManifestUtils.h"

namespace facebook::fboss::configgen {
namespace {

TEST(ConfigGenerationManifestUtilsTest, ParsesEnumNamesIntoTypedManifest) {
  const auto manifest = parseConfigGenerationManifest(
      R"({"targets":[{"platform":"PLATFORM_MONTBLANC","serviceToOptions":{"AGENT":[{"profile":"DEFAULT","variant":"example"}]}}]})");

  const auto& target = manifest.targets()->at(0);
  EXPECT_EQ(*target.platform(), PlatformType::PLATFORM_MONTBLANC);
  const auto& option = target.serviceToOptions()->at(ServiceType::AGENT).at(0);
  EXPECT_EQ(*option.profile(), ConfigProfileType::DEFAULT);
  EXPECT_EQ(*option.variant(), "example");
}

TEST(ConfigGenerationManifestUtilsTest, RejectsInvalidManifests) {
  const std::vector<std::string_view> invalidManifests{
      R"({"targets":[]})",
      R"({"targets":[{"platform":"PLATFORM_MONTBLANC","serviceToOptions":{}}]})",
      R"({"targets":[{"platform":"PLATFORM_MONTBLANC","serviceToOptions":{"AGENT":[]}}]})",
      R"({"targets":[{"platform":"PLATFORM_MONTBLANC","serviceToOptions":{"AGENT":[{"profile":"DEFAULT","variant":""}]}}]})",
      R"({"targets":[{"platform":"PLATFORM_MONTBLANC","serviceToOptions":{"AGENT":[{"profile":"DEFAULT"}]}},{"platform":"PLATFORM_MONTBLANC","serviceToOptions":{"AGENT":[{"profile":"HW_TEST"}]}}]})",
      R"({"targets":[{"platform":"PLATFORM_MONTBLANC","serviceToOptions":{"AGENT":[{"profile":"DEFAULT"},{"profile":"DEFAULT"}]}}]})",
      R"({"targets":[{"platform":"PLATFORM_MONTBLANC","serviceToOptions":{"AGENT":[{"profile":"DEFAULT","unknown":true}]}}]})",
      R"({"targets":[{"platform":"NOT_A_PLATFORM","serviceToOptions":{"AGENT":[{"profile":"DEFAULT"}]}}]})",
      R"({"targets":[{"platform":"PLATFORM_MONTBLANC","serviceToOptions":{"NOT_A_SERVICE":[{"profile":"DEFAULT"}]}}]})",
      R"({"targets":[{"platform":"PLATFORM_MONTBLANC","serviceToOptions":{"AGENT":[{"profile":"NOT_A_PROFILE"}]}}]})",
      R"({"targets":[{"platform":76,"serviceToOptions":{"AGENT":[{"profile":"DEFAULT"}]}}]})",
  };

  for (const auto manifest : invalidManifests) {
    EXPECT_THROW(parseConfigGenerationManifest(manifest), std::exception)
        << manifest;
  }
}

} // namespace
} // namespace facebook::fboss::configgen
