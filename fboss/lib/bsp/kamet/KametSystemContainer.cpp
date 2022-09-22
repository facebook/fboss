// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/kamet/KametSystemContainer.h"
#include <folly/Singleton.h>
#include "fboss/lib/bsp/kamet/KametBspPlatformMapping.h"

namespace {
static facebook::fboss::BspPlatformMapping* kKametMapping =
    new facebook::fboss::KametBspPlatformMapping();
} // namespace

namespace facebook {
namespace fboss {

KametSystemContainer::KametSystemContainer()
    : BspSystemContainer(kKametMapping) {}

folly::Singleton<KametSystemContainer> systemContainer_;

std::shared_ptr<KametSystemContainer> KametSystemContainer::getInstance() {
  return systemContainer_.try_get();
}

} // namespace fboss
} // namespace facebook
