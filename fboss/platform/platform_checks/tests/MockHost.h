/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <gmock/gmock.h>

#include "fboss/platform/platform_checks/Host.h"

namespace facebook::fboss::platform::platform_checks {

class MockHost : public Host {
 public:
  explicit MockHost(std::string name = "mock", bool isLocal = false)
      : name_(std::move(name)), isLocal_(isLocal) {}

  std::string name() const override {
    return name_;
  }

  bool isLocal() const override {
    return isLocal_;
  }

  MOCK_METHOD(
      CommandResult,
      run,
      (const std::string&, std::chrono::seconds),
      (const, override));
  MOCK_METHOD(
      void,
      copyTo,
      (const std::filesystem::path&,
       const std::filesystem::path&,
       std::chrono::seconds),
      (const, override));
  MOCK_METHOD(
      std::optional<std::string>,
      readFile,
      (const std::filesystem::path&),
      (const, override));
  MOCK_METHOD(bool, exists, (const std::filesystem::path&), (const, override));
  MOCK_METHOD(
      std::vector<std::filesystem::path>,
      listDirectory,
      (const std::filesystem::path&),
      (const, override));

 private:
  std::string name_;
  bool isLocal_;
};

} // namespace facebook::fboss::platform::platform_checks
