// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/BspPimContainer.h"
#include "fboss/lib/bsp/BspTransceiverContainer.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

namespace facebook {
namespace fboss {

BspPimContainer::BspPimContainer(BspPimMapping& bspPimMapping)
    : bspPimMapping_(bspPimMapping) {
  for (auto tcvrMapping : *bspPimMapping.tcvrMapping()) {
    tcvrContainers_.emplace(
        tcvrMapping.first,
        std::make_unique<BspTransceiverContainer>(tcvrMapping.second));
  }
}

const BspTransceiverContainer* BspPimContainer::getTransceiverContainer(
    int tcvrID) const {
  return tcvrContainers_.at(tcvrID).get();
}

void BspPimContainer::initAllTransceivers() const {
  for (auto tcvrContainerIt = tcvrContainers_.begin();
       tcvrContainerIt != tcvrContainers_.end();
       tcvrContainerIt++) {
    tcvrContainerIt->second->initTransceiver();
  }
}

void BspPimContainer::clearAllTransceiverReset() const {
  for (auto tcvrContainerIt = tcvrContainers_.begin();
       tcvrContainerIt != tcvrContainers_.end();
       tcvrContainerIt++) {
    tcvrContainerIt->second->clearTransceiverReset();
  }
}

void BspPimContainer::triggerTcvrHardReset(int tcvrID) const {
  getTransceiverContainer(tcvrID)->triggerTcvrHardReset();
}

bool BspPimContainer::isTcvrPresent(int tcvrID) const {
  return getTransceiverContainer(tcvrID)->isTcvrPresent();
}

void BspPimContainer::tcvrRead(
    unsigned int tcvrID,
    const TransceiverAccessParameter& param,
    uint8_t* buf) const {
  return getTransceiverContainer(tcvrID)->tcvrRead(param, buf);
}

void BspPimContainer::tcvrWrite(
    unsigned int tcvrID,
    const TransceiverAccessParameter& param,
    const uint8_t* buf) const {
  return getTransceiverContainer(tcvrID)->tcvrWrite(param, buf);
}

} // namespace fboss
} // namespace facebook
