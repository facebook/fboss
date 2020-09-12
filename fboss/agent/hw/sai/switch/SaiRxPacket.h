/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"

#include "fboss/agent/RxPacket.h"

namespace facebook::fboss {

class SaiRxPacket : public RxPacket {
 public:
  explicit SaiRxPacket(
      size_t buffer_size,
      const void* buffer,
      PortID portID,
      VlanID vlanID);
  /*
   * Set the port on which this packet was received.
   */
  void setSrcPort(PortID srcPort) {
    srcPort_ = srcPort;
  }
  /*
   * Set the VLAN on which this packet was received.
   */
  void setSrcVlan(VlanID srcVlan) {
    srcVlan_ = srcVlan;
  }
};
} // namespace facebook::fboss
