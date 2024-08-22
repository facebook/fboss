// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <gmock/gmock.h>

#include "fboss/platform/helpers/PlatformFsUtils.h"

namespace facebook::fboss::platform {
class MockPlatformFsUtils : public PlatformFsUtils {
 public:
  MOCK_METHOD(
      (std::optional<std::string>),
      getStringFileContent,
      (const std::string&),
      (const));
};
} // namespace facebook::fboss::platform
