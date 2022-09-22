// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspSystemContainer.h"
#include "fboss/lib/bsp/kamet/KametBspPlatformMapping.h"

namespace facebook::fboss {

class KametSystemContainer : public BspSystemContainer {
 public:
  KametSystemContainer();
  static std::shared_ptr<KametSystemContainer> getInstance();
};

} // namespace facebook::fboss
