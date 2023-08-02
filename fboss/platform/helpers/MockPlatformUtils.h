// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <gmock/gmock.h>

#include "fboss/platform/helpers/PlatformUtils.h"

namespace facebook::fboss::platform {
class MockPlatformUtils : public PlatformUtils {
 public:
  MOCK_METHOD(
      (std::pair<int, std::string>),
      execCommand,
      (const std::string&),
      (const));
};
} // namespace facebook::fboss::platform
