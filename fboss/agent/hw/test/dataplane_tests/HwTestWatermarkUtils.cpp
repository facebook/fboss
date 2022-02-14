/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/dataplane_tests/HwTestWatermarkUtils.h"
#include "fboss/agent/hw/test/ConfigFactory.h"

namespace facebook::fboss::utility {

size_t getEffectiveBytesPerPacket(HwSwitch* hwSwitch, int pktSizeBytes) {
  auto packetBufferUnitBytes =
      hwSwitch->getPlatform()->getAsic()->getPacketBufferUnitSize();
  auto bufferDescriptorBytes =
      hwSwitch->getPlatform()->getAsic()->getPacketBufferDescriptorSize();

  assert(packetBufferUnitBytes);
  size_t effectiveBytesPerPkt = packetBufferUnitBytes *
      ((pktSizeBytes + bufferDescriptorBytes + packetBufferUnitBytes - 1) /
       packetBufferUnitBytes);
  return effectiveBytesPerPkt;
}

}; // namespace facebook::fboss::utility
