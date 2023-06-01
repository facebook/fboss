// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/BspSystemContainer.h"
#include "fboss/lib/bsp/BspPimContainer.h"
#include "fboss/lib/bsp/BspPlatformMapping.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"
#include "fboss/lib/fpga/FpgaDevice.h"

namespace facebook {
namespace fboss {

BspSystemContainer::BspSystemContainer(std::unique_ptr<FpgaDevice> fpgaDevice)
    : fpgaDevice_(std::move(fpgaDevice)) {
  fpgaDevice_->mmap();
}

BspSystemContainer::BspSystemContainer(BspPlatformMapping* bspMapping)
    : bspMapping_(bspMapping) {
  initializePimContainers();
}

void BspSystemContainer::initializePimContainers() {
  for (auto pimMapping : bspMapping_->getPimMappings()) {
    pimContainers_.emplace(
        pimMapping.first, std::make_unique<BspPimContainer>(pimMapping.second));
  }
}

const BspPimContainer* BspSystemContainer::getPimContainerFromPimID(
    int pimID) const {
  CHECK(pimContainers_.find(pimID) != pimContainers_.end());
  return pimContainers_.at(pimID).get();
}

const BspPimContainer* BspSystemContainer::getPimContainerFromTcvrID(
    int tcvrID) const {
  CHECK(tcvrID >= 1 && tcvrID <= getNumTransceivers());
  auto pimID = getPimIDFromTcvrID(tcvrID);
  return getPimContainerFromPimID(pimID);
}

int BspSystemContainer::getNumTransceivers() const {
  return bspMapping_->numTransceivers();
}

int BspSystemContainer::getPimIDFromTcvrID(int tcvrID) const {
  return bspMapping_->getPimIDFromTcvrID(tcvrID);
}

int BspSystemContainer::getNumPims() const {
  return bspMapping_->numPims();
}

void BspSystemContainer::initAllTransceivers() const {
  for (auto pimContainerIt = pimContainers_.begin();
       pimContainerIt != pimContainers_.end();
       pimContainerIt++) {
    pimContainerIt->second->initAllTransceivers();
  }
}

void BspSystemContainer::clearAllTransceiverReset() const {
  for (auto pimContainerIt = pimContainers_.begin();
       pimContainerIt != pimContainers_.end();
       pimContainerIt++) {
    pimContainerIt->second->clearAllTransceiverReset();
  }
}

void BspSystemContainer::triggerQsfpHardReset(int tcvrID) const {
  getPimContainerFromTcvrID(tcvrID)->triggerTcvrHardReset(tcvrID);
}

bool BspSystemContainer::isTcvrPresent(int tcvrID) const {
  return getPimContainerFromTcvrID(tcvrID)->isTcvrPresent(tcvrID);
}

void BspSystemContainer::tcvrRead(
    unsigned int tcvrID,
    const TransceiverAccessParameter& param,
    uint8_t* buf) const {
  return getPimContainerFromTcvrID(tcvrID)->tcvrRead(tcvrID, param, buf);
}

void BspSystemContainer::tcvrWrite(
    unsigned int tcvrID,
    const TransceiverAccessParameter& param,
    const uint8_t* buf) const {
  return getPimContainerFromTcvrID(tcvrID)->tcvrWrite(tcvrID, param, buf);
}

const I2cControllerStats BspSystemContainer::getI2cControllerStats(
    int tcvrID) const {
  return getPimContainerFromTcvrID(tcvrID)->getI2cControllerStats(tcvrID);
}

std::map<uint32_t, LedIO*> BspSystemContainer::getLedController(
    int tcvrID) const {
  std::map<uint32_t, LedIO*> ledControllers;

  auto ledContainers =
      getPimContainerFromTcvrID(tcvrID)->getLedContainer(tcvrID);
  for (auto& ledContainer : ledContainers) {
    ledControllers[ledContainer.first] =
        ledContainer.second->getLedController();
  }
  return ledControllers;
}

BspDeviceMdioController* BspSystemContainer::getMdioController(
    int pimID,
    int controllerID) const {
  return getPimContainerFromPimID(pimID)
      ->getPhyContainerFromMdioID(controllerID)
      ->getMdioController();
}

} // namespace fboss
} // namespace facebook
