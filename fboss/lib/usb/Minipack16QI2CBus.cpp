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
#include "fboss/lib/fpga/MinipackFpga.h"

#include <folly/container/Enumerate.h>
#include <folly/logging/xlog.h>

namespace {

enum Minipack16QPimMuxes {
  MUX_PIM_SELECT = 0x70,
  MUX_0_TO_7 = 0x72,
  MUX_8_TO_15 = 0x71,
};

constexpr uint32_t kPortsPerPim = 16;

inline uint8_t getPim(int module) {
  return 1 + (module - 1) / 16;
}

inline uint8_t getQsfpPimPort(int module) {
  return (module - 1) % 16;
}

inline int getModule(uint8_t pim, uint8_t port) {
  return 1 + (pim - 1) * kPortsPerPim + port;
}
} // namespace

namespace facebook::fboss {
Minipack16QI2CBus::Minipack16QI2CBus() {
  // Initialize the MinipackFpga Singleton.
  MinipackFpga::getInstance()->initHW();

  // Initialize the real time I2C access controllers.
  for (uint32_t pim = 1; pim <= MinipackFpga::kNumberPim; ++pim) {
    for (uint32_t rtc = 0; rtc < 4; rtc++) {
      i2cControllers_[pim - 1][rtc] = std::make_unique<FbFpgaI2cController>(
          MinipackFpga::getInstance()->getDomFpga(pim), rtc, pim);
    }
  }
}

Minipack16QI2CBus::~Minipack16QI2CBus() {}

void Minipack16QI2CBus::moduleRead(
    unsigned int module,
    uint8_t /* i2cAddress */,
    int offset,
    int len,
    uint8_t* buf) {
  if (len > 128) {
    throw MinipackI2cError("Too long read");
  }
  auto pim = getPim(module);
  auto port = getQsfpPimPort(module);

  XLOG(DBG3) << folly::format(
      "I2C read to pim {:d}, port {:d} at offset {:#x} for {:d} bytes",
      pim,
      port,
      offset,
      len);

  getI2cController(pim, getI2cControllerIdx(port))
      ->read(
          getI2cControllerChannel(port),
          offset,
          folly::MutableByteRange(buf, len));
}

void Minipack16QI2CBus::moduleWrite(
    unsigned int module,
    uint8_t /* i2cAddress */,
    int offset,
    int len,
    const uint8_t* data) {
  if (len > 128) {
    throw MinipackI2cError("Too long write");
  }
  auto pim = getPim(module);
  auto port = getQsfpPimPort(module);

  XLOG(DBG3) << folly::format(
      "I2C write to pim {:d}, port {:d} at offset {:#x} for {:d} bytes",
      pim,
      port,
      offset,
      len);

  getI2cController(pim, getI2cControllerIdx(port))
      ->write(
          getI2cControllerChannel(port), offset, folly::ByteRange(data, len));
}

bool Minipack16QI2CBus::isPresent(unsigned int module) {
  auto pim = getPim(module);
  auto port = getQsfpPimPort(module);
  XLOG(DBG5) << folly::format(
      "detecting presence of qsfp at pim:{:d}, port:{:d}", pim, port);
  return MinipackFpga::getInstance()->isQsfpPresent(pim, port);
}

void Minipack16QI2CBus::scanPresence(
    std::map<int32_t, ModulePresence>& presences) {
  std::set<uint8_t> pimsToScan;
  for (auto presence : presences) {
    pimsToScan.insert(getPim(presence.first + 1));
  }

  for (auto pim : pimsToScan) {
    auto pimQsfpPresence = MinipackFpga::getInstance()->scanQsfpPresence(pim);
    for (int port = 0; port < kPortsPerPim; port++) {
      if (pimQsfpPresence[port]) {
        presences[getModule(pim, port) - 1] = ModulePresence::PRESENT;
      } else {
        presences[getModule(pim, port) - 1] = ModulePresence::ABSENT;
      }
    }
  }
}

void Minipack16QI2CBus::ensureOutOfReset(unsigned int module) {
  auto pim = getPim(module);
  auto port = getQsfpPimPort(module);
  MinipackFpga::getInstance()->ensureQsfpOutOfReset(pim, port);
}

folly::EventBase* Minipack16QI2CBus::getEventBase(unsigned int module) {
  return i2cControllers_[getPim(module) - 1]
                        [getI2cControllerIdx(getQsfpPimPort(module))]
                            ->getEventBase();
}

FbFpgaI2cController* Minipack16QI2CBus::getI2cController(
    uint8_t pim,
    uint8_t idx) const {
  return i2cControllers_[pim - 1][idx].get();
}

/* Consolidate the i2c transaction stats from all the pims using their
 * corresponding i2c controller. In case of Minipack16q there are 8 pims
 * and there are four FbFpgaI2cController corresponding to each pim. This
 * function consolidates the counters from all constollers and return the
 * array of the i2c stats
 */
std::vector<std::reference_wrapper<const I2cControllerStats>>
Minipack16QI2CBus::getI2cControllerStats() {
  std::vector<std::reference_wrapper<const I2cControllerStats>>
      i2cControllerCurrentStats;

  for (uint32_t pim = 1; pim <= MinipackFpga::kNumberPim; ++pim) {
    for (uint32_t idx = 0; idx < 4; idx++) {
      i2cControllerCurrentStats.push_back(
          getI2cController(pim, idx)->getI2cControllerPlatformStats());
    }
  }
  return i2cControllerCurrentStats;
}

} // namespace facebook::fboss
