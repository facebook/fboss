// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.

#pragma once

#include <folly/experimental/FunctionScheduler.h>

namespace facebook::fboss::platform::platform_manager {
class PlatformExplorer {
 public:
  explicit PlatformExplorer(std::chrono::seconds exploreInterval);
  void explore();

 private:
  folly::FunctionScheduler scheduler_;
};

} // namespace facebook::fboss::platform::platform_manager
