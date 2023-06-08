// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <gmock/gmock.h>

#include "fboss/platform/config_lib/ConfigLib.h"

namespace facebook::fboss::platform {

class MockConfigLib : public ConfigLib {
 public:
  MOCK_METHOD(
      std::string,
      getSensorServiceConfig,
      (const std::optional<std::string>&),
      (const));

  MOCK_METHOD(
      std::string,
      getFanServiceConfig,
      (const std::optional<std::string>&),
      (const));

  MOCK_METHOD(
      std::string,
      getPlatformManagerConfig,
      (const std::optional<std::string>&),
      (const));

  MOCK_METHOD(
      std::string,
      getWeutilConfig,
      (const std::optional<std::string>&),
      (const));

  MOCK_METHOD(
      std::string,
      getFwUtilConfig,
      (const std::optional<std::string>&),
      (const));

 private:
};

} // namespace facebook::fboss::platform
