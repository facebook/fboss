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

#include "fboss/agent/Packet.h"
#include "fboss/agent/types.h"

#include <string>

namespace facebook { namespace fboss {

/*
 * RxPacket represents a packet that was received via one of the switch ports.
 */
class RxPacket : public Packet {
 public:
  /*
   * Get the port on which this packet was received.
   */
  PortID getSrcPort() const {
    return srcPort_;
  }
  /*
   * Get the VLAN on which this packet was received.
   */
  VlanID getSrcVlan() const {
    return srcVlan_;
  }
  /**
   * Get the length of the packet
   */
  uint32_t getLength() const {
    return len_;
  }
  /**
   * Get the router ID of the packet
   */
  RouterID getRouterID() const {
    // TODO: only vrf 0 now
    return RouterID(0);
  }

  /*
   * Return a human-readable string describing additional detailed information
   * about the packet.
   *
   * This allows hardware-specific packet details to be included in log
   * messages.
   */
  virtual std::string describeDetails() const {
    return "";
  }

 protected:
  PortID srcPort_{0};
  VlanID srcVlan_{0};
  uint32_t len_{0};
};

}} // facebook::fboss
