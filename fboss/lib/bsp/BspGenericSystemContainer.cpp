// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/kamet/KametBspPlatformMapping.h"
#include "fboss/lib/bsp/meru400biu/Meru400biuBspPlatformMapping.h"

namespace facebook {
namespace fboss {

using KametSystemContainer = BspGenericSystemContainer<KametBspPlatformMapping>;
folly::Singleton<KametSystemContainer> _kametSystemContainer;
template <>
std::shared_ptr<KametSystemContainer> KametSystemContainer::getInstance() {
  return _kametSystemContainer.try_get();
}

using Meru400biuSystemContainer =
    BspGenericSystemContainer<Meru400biuBspPlatformMapping>;
folly::Singleton<Meru400biuSystemContainer> _meru400biuSystemContainer;
template <>
std::shared_ptr<Meru400biuSystemContainer>
Meru400biuSystemContainer::getInstance() {
  return _meru400biuSystemContainer.try_get();
}

} // namespace fboss
} // namespace facebook
