// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/BspPimContainer.h"
#include "fboss/agent/FbossError.h"
#include "fboss/lib/bsp/BspLedContainer.h"
#include "fboss/lib/bsp/BspPhyContainer.h"
#include "fboss/lib/bsp/BspTransceiverContainer.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

namespace facebook {
namespace fboss {

BspPimContainer::BspPimContainer(BspPimMapping& bspPimMapping)
    : bspPimMapping_(bspPimMapping) {
  for (auto tcvrMapping : *bspPimMapping.tcvrMapping()) {
    auto ioControllerId = *tcvrMapping.second.io()->controllerId();
    // Create an event base and thread if not already created for this IO
    // controller
    if (ioToEvbThread_.find(ioControllerId) == ioToEvbThread_.end()) {
      ioToEvbThread_[ioControllerId] = {};
      ioToEvbThread_[ioControllerId].first =
          std::make_unique<folly::EventBase>();
      auto evb = ioToEvbThread_[ioControllerId].first.get();
      ioToEvbThread_[ioControllerId].second =
          std::make_unique<std::thread>([evb] { evb->loopForever(); });
      XLOG(DBG3) << "Created EVB for " << ioControllerId;
    }
    tcvrToIOEvb_[tcvrMapping.first] =
        ioToEvbThread_[ioControllerId].first.get();
    tcvrContainers_.emplace(
        tcvrMapping.first,
        std::make_unique<BspTransceiverContainer>(tcvrMapping.second));
  }
  // Create PhyIO controllers
  for (auto controller : *bspPimMapping.phyIOControllers()) {
    phyIOControllers_[controller.first] =
        std::make_unique<BspPhyIO>(*bspPimMapping.pimID(), controller.second);
  }
  // Create PHY containers, 1 per PHY in a PIM
  for (auto phyMapping : *bspPimMapping.phyMapping()) {
    CHECK(
        phyIOControllers_.find(*phyMapping.second.phyIOControllerId()) !=
        phyIOControllers_.end());
    phyContainers_.emplace(
        phyMapping.first,
        std::make_unique<BspPhyContainer>(
            *bspPimMapping.pimID(),
            phyMapping.second,
            phyIOControllers_[*phyMapping.second.phyIOControllerId()].get()));
  }
  for (auto ledMapping : *bspPimMapping.ledMapping()) {
    ledContainers_.emplace(
        ledMapping.first, std::make_unique<BspLedContainer>(ledMapping.second));
  }
}

const BspTransceiverContainer* BspPimContainer::getTransceiverContainer(
    int tcvrID) const {
  return tcvrContainers_.at(tcvrID).get();
}

const BspPhyContainer* BspPimContainer::getPhyContainerFromPhyID(
    int phyID) const {
  return phyContainers_.at(phyID).get();
}

const BspPhyContainer* BspPimContainer::getPhyContainerFromMdioID(
    int mdioControllerID) const {
  for (auto phy : *bspPimMapping_.phyMapping()) {
    if (mdioControllerID == *phy.second.phyIOControllerId()) {
      return phyContainers_.at(phy.first).get();
    }
  }
  throw FbossError(fmt::format(
      "Couldn't find phy container for mdioID {:d}, PimID {:d}",
      mdioControllerID,
      *bspPimMapping_.pimID()));
}

const std::map<uint32_t, const BspLedContainer*>
BspPimContainer::getLedContainer(int tcvrID) const {
  std::map<uint32_t, const BspLedContainer*> ledContainers;

  if (bspPimMapping_.tcvrMapping().value().find(tcvrID) ==
      bspPimMapping_.tcvrMapping().value().end()) {
    XLOG(ERR) << "Transceiver mapping could not be found for " << tcvrID;
    return {};
  }

  for (auto tcvrLaneToLed : bspPimMapping_.tcvrMapping()
                                .value()
                                .at(tcvrID)
                                .tcvrLaneToLedId()
                                .value()) {
    uint32_t ledId = tcvrLaneToLed.second;
    if (ledContainers.find(ledId) == ledContainers.end()) {
      ledContainers[ledId] = ledContainers_.at(ledId).get();
    }
  }
  return ledContainers;
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

const I2cControllerStats BspPimContainer::getI2cControllerStats(
    int tcvrID) const {
  return getTransceiverContainer(tcvrID)->getI2cControllerStats();
}

BspPimContainer::~BspPimContainer() {
  for (auto& evb : ioToEvbThread_) {
    evb.second.first->terminateLoopSoon();
    evb.second.second->join();
  }
}

} // namespace fboss
} // namespace facebook
