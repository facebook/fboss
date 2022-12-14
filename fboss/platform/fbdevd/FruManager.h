// Copyright (c) Meta Platforms, Inc. and affiliates.

#pragma once

#include <string>

#include "fboss/platform/fbdevd/if/gen-cpp2/FbdevManager.h"

namespace facebook::fboss::platform::fbdevd {

class FruManager {
 public:
  explicit FruManager(const FruModule& fru);
  ~FruManager() {}

  // Test if the FRU is present/connected or not.
  bool isPresent();

  // processEvents() function need to be triggered periodically to detect
  // and handle FRU hotplug events.
  void processEvents();

 private:
  bool isPresent_ = false; // Assume FRU is unplugged at "fbdevd" starts.
};
} // namespace facebook::fboss::platform::fbdevd
