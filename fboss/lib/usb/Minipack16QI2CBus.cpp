/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/usb/Minipack16QI2CBus.h"
#include "fboss/agent/Utils.h"
#include "fboss/lib/fpga/MinipackSystemContainer.h"

#include <folly/container/Enumerate.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss {
Minipack16QI2CBus::Minipack16QI2CBus() {
  systemContainer_ = MinipackSystemContainer::getInstance();
  systemContainer_->initHW();
}

Minipack16QI2CBus::~Minipack16QI2CBus() {}

void Minipack16QI2CBus::ensureOutOfReset(unsigned int module) {
  auto pim = getPim(module);
  auto port = getQsfpPimPort(module);
  systemContainer_->getPimContainer(pim)
      ->getPimQsfpController()
      ->ensureQsfpOutOfReset(port);
}

folly::EventBase* Minipack16QI2CBus::getEventBase(unsigned int module) {
  auto pim = getPim(module);
  auto port = getQsfpPimPort(module);
  return systemContainer_->getPimContainer(pim)
      ->getI2cController(port)
      ->getEventBase();
}

FbFpgaI2cController* Minipack16QI2CBus::getI2cController(
    uint8_t pim,
    uint8_t idx) const {
  return systemContainer_->getPimContainer(pim)->getI2cController(idx);
}

/* Consolidate the i2c transaction stats from all the pims using their
 * corresponding i2c controller. In case of Minipack16q there are 8 pims
 * and there are four FbFpgaI2cController corresponding to each pim. This
 * function consolidates the counters from all constollers and return the
 * array of the i2c stats
 */
std::vector<I2cControllerStats> Minipack16QI2CBus::getI2cControllerStats() {
  std::vector<I2cControllerStats> i2cControllerCurrentStats;

  for (uint32_t pim = MinipackSystemContainer::kPimStartNum;
       pim < MinipackSystemContainer::kPimStartNum +
           MinipackSystemContainer::kNumberPim;
       ++pim) {
    for (uint32_t idx = 0; idx < 4; idx++) {
      i2cControllerCurrentStats.push_back(
          getI2cController(pim, idx)->getI2cControllerPlatformStats());
    }
  }
  return i2cControllerCurrentStats;
}

} // namespace facebook::fboss
