// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once
#include <memory>

#include <folly/Singleton.h>
#include "fboss/lib/bsp/BspPlatformMapping.h"
#include "fboss/lib/bsp/BspSystemContainer.h"

namespace facebook {
namespace fboss {

template <typename T>
class BspGenericSystemContainer : public BspSystemContainer {
 public:
  BspGenericSystemContainer() : BspSystemContainer(initBspPlatformMapping()) {}
  static std::shared_ptr<BspGenericSystemContainer> getInstance();

 private:
  std::unique_ptr<BspPlatformMapping> initBspPlatformMapping();
};

} // namespace fboss
} // namespace facebook
