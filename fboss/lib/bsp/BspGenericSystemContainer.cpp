// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/meru400bfu/Meru400bfuBspPlatformMapping.h"
#include "fboss/lib/bsp/meru400biu/Meru400biuBspPlatformMapping.h"
#include "fboss/lib/bsp/montblanc/MontblancBspPlatformMapping.h"

namespace facebook {
namespace fboss {

using Meru400bfuSystemContainer =
    BspGenericSystemContainer<Meru400bfuBspPlatformMapping>;
folly::Singleton<Meru400bfuSystemContainer> _meru400bfuSystemContainer;
template <>
std::shared_ptr<Meru400bfuSystemContainer>
Meru400bfuSystemContainer::getInstance() {
  return _meru400bfuSystemContainer.try_get();
}

using Meru400biuSystemContainer =
    BspGenericSystemContainer<Meru400biuBspPlatformMapping>;
folly::Singleton<Meru400biuSystemContainer> _meru400biuSystemContainer;
template <>
std::shared_ptr<Meru400biuSystemContainer>
Meru400biuSystemContainer::getInstance() {
  return _meru400biuSystemContainer.try_get();
}

using MontblancSystemContainer =
    BspGenericSystemContainer<MontblancBspPlatformMapping>;
folly::Singleton<MontblancSystemContainer> _montblancSystemContainer;
template <>
std::shared_ptr<MontblancSystemContainer>
MontblancSystemContainer::getInstance() {
  return _montblancSystemContainer.try_get();
}

} // namespace fboss
} // namespace facebook
