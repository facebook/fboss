// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once
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
  BspPlatformMapping* initBspPlatformMapping() {
    bspPlatformMapping_ = new T();
    return bspPlatformMapping_;
  }

  T* bspPlatformMapping_;
};

} // namespace fboss
} // namespace facebook
