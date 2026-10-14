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

TEST(ConfigGenerationManifestUtilsTest, ParsesEnumNamesIntoTypedTarget) {
  const auto target = parseConfigGenerationTarget(
      R"({"platform":"PLATFORM_MONTBLANC","serviceToOptions":{"AGENT":[{"profile":"BENCHMARK","variant":"example","asicConfigVariant":"default"}]}})");

  EXPECT_EQ(*target.platform(), PlatformType::PLATFORM_MONTBLANC);
  const auto& option = target.serviceToOptions()->at(ServiceType::AGENT).at(0);
  EXPECT_EQ(*option.profile(), ConfigProfileType::BENCHMARK);
  EXPECT_EQ(*option.variant(), "example");
  EXPECT_EQ(*option.asicConfigVariant(), "default");
}

TEST(ConfigGenerationManifestUtilsTest, RejectsInvalidTargets) {
  const std::vector<std::string_view> invalidTargets{
      R"({"platform":"PLATFORM_MONTBLANC","serviceToOptions":{}})",
      R"({"platform":"PLATFORM_MONTBLANC","serviceToOptions":{"AGENT":[]}})",
      R"({"platform":"PLATFORM_MONTBLANC","serviceToOptions":{"AGENT":[{"profile":"DEFAULT","variant":""}]}})",
      R"({"platform":"PLATFORM_MONTBLANC","serviceToOptions":{"AGENT":[{"profile":"DEFAULT","asicConfigVariant":""}]}})",
      R"({"platform":"PLATFORM_MONTBLANC","serviceToOptions":{"AGENT":[{"profile":"DEFAULT"},{"profile":"DEFAULT"}]}})",
      R"({"platform":"PLATFORM_MONTBLANC","serviceToOptions":{"AGENT":[{"profile":"DEFAULT","unknown":true}]}})",
      R"({"platform":"NOT_A_PLATFORM","serviceToOptions":{"AGENT":[{"profile":"DEFAULT"}]}})",
      R"({"platform":"PLATFORM_MONTBLANC","serviceToOptions":{"NOT_A_SERVICE":[{"profile":"DEFAULT"}]}})",
      R"({"platform":"PLATFORM_MONTBLANC","serviceToOptions":{"AGENT":[{"profile":"NOT_A_PROFILE"}]}})",
      R"({"platform":76,"serviceToOptions":{"AGENT":[{"profile":"DEFAULT"}]}})",
  };

  for (const auto target : invalidTargets) {
    EXPECT_THROW(parseConfigGenerationTarget(target), std::exception) << target;
  }
}

} // namespace
} // namespace facebook::fboss::configgen
