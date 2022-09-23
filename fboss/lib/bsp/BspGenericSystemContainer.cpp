// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/kamet/KametBspPlatformMapping.h"

namespace facebook {
namespace fboss {

using KametSystemContainer = BspGenericSystemContainer<KametBspPlatformMapping>;
folly::Singleton<KametSystemContainer> _kametSystemContainer;
template <>
std::shared_ptr<KametSystemContainer> KametSystemContainer::getInstance() {
  return _kametSystemContainer.try_get();
}

} // namespace fboss
} // namespace facebook
