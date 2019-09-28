/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/usb/Wedge100I2CBus.h"

#include <folly/container/Enumerate.h>
#include <folly/logging/xlog.h>

namespace {

enum Wedge100Muxes {
  MUX_0_TO_7 = 0x70,
  MUX_8_TO_15 = 0x71,
  MUX_16_TO_23 = 0x72,
  MUX_24_TO_31 = 0x73,
  MUX_PCA9548_5 = 0x74
};

enum Wedge100StatusMuxChannels { PRSNT_N_LOW = 2, PRSNT_N_HIGH = 3 };

// Addresses unique to wedge100:
enum : uint8_t { ADDR_QSFP_PRSNT_LOWER = 0x22, ADDR_QSFP_PRSNT_UPPER = 0x23 };

void extractPresenceBits(
    uint8_t* buf,
    std::map<int32_t, facebook::fboss::ModulePresence>& presences,
    bool upperQsfps) {
  for (int byte = 0; byte < 2; byte++) {
    for (int bit = 0; bit < 8; bit++) {
      int txcvIdx = byte * 8 + bit;
      if (upperQsfps) {
        txcvIdx += 16;
      }
      ((buf[byte] >> bit) & 1)
          ? presences[txcvIdx] = facebook::fboss::ModulePresence::ABSENT
          : presences[txcvIdx] = facebook::fboss::ModulePresence::PRESENT;
    }
  }
}
} // namespace

namespace facebook {
namespace fboss {
void Wedge100I2CBus::scanPresence(
    std::map<int32_t, ModulePresence>& presences) {
  // If selectedPort_ != NO_PORT, then unselectAll
  if (selectedPort_) {
    selectQsfpImpl(NO_PORT);
  }
  uint8_t buf[2];
  try {
    read(ADDR_QSFP_PRSNT_LOWER, 0, 2, buf);
    extractPresenceBits(buf, presences, false);
  } catch (const I2cError& ex) {
    XLOG(ERR) << "Error reading lower QSFP presence bits";
  }

  try {
    read(ADDR_QSFP_PRSNT_UPPER, 0, 2, buf);
    extractPresenceBits(buf, presences, true);
  } catch (const I2cError& ex) {
    XLOG(ERR) << "Error reading upper QSFP presence bits";
  }
}

void Wedge100I2CBus::initBus() {
  PCA9548MuxedBus<32>::initBus();
  qsfpStatusMux_ = std::make_unique<PCA9548>(dev_.get(), MUX_PCA9548_5);
  qsfpStatusMux_->select((1 << PRSNT_N_LOW) + (1 << PRSNT_N_HIGH));
}

MuxLayer Wedge100I2CBus::createMuxes() {
  MuxLayer muxes;
  muxes.push_back(std::make_unique<QsfpMux>(dev_.get(), MUX_0_TO_7));
  muxes.push_back(std::make_unique<QsfpMux>(dev_.get(), MUX_8_TO_15));
  muxes.push_back(std::make_unique<QsfpMux>(dev_.get(), MUX_16_TO_23));
  muxes.push_back(std::make_unique<QsfpMux>(dev_.get(), MUX_24_TO_31));
  return muxes;
}

void Wedge100I2CBus::wireUpPorts(Wedge100I2CBus::PortLeaves& leaves) {
  for (auto&& mux : folly::enumerate(roots_)) {
    connectPortsToMux(leaves, (*mux).get(), mux.index * PCA9548::WIDTH, true);
  }
}

} // namespace fboss
} // namespace facebook
