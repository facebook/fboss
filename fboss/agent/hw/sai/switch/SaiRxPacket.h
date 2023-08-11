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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"

#include "fboss/agent/RxPacket.h"

namespace facebook::fboss {

class SaiRxPacket : public RxPacket {
 public:
  SaiRxPacket(
      size_t buffer_size,
      const void* buffer,
      PortID portID,
      VlanID vlanID,
      cfg::PacketRxReason rxReason,
      uint8_t queueId);

  SaiRxPacket(
      size_t buffer_size,
      const void* buffer,
      AggregatePortID aggregatePortID,
      VlanID vlanID,
      cfg::PacketRxReason rxReason,
      uint8_t queueId);
  /*
   * Set the port on which this packet was received.
   */
  void setSrcPort(PortID srcPort) {
    srcPort_ = srcPort;
  }
  /*
   * Set the VLAN on which this packet was received.
   */
  void setSrcVlan(std::optional<VlanID> srcVlan) {
    srcVlan_ = srcVlan;
  }

  /*
   * Set the aggregate port on which this packet was received.
   */
  void setSrcAggregatePort(AggregatePortID srcAggregatePort) {
    isFromAggregatePort_ = true;
    srcAggregatePort_ = srcAggregatePort;
  }

  std::string describeDetails() const override;

 private:
  cfg::PacketRxReason rxReason_;
};
} // namespace facebook::fboss
