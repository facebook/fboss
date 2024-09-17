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
#include <tuple>
#include <vector>

namespace facebook::fboss {

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
   * Return True if and only if the packet ingressed on an aggregate port.
   */
  bool isFromAggregatePort() const {
    return isFromAggregatePort_;
  }
  /*
   * Get the aggregate port on which this packet was received.
   */
  AggregatePortID getSrcAggregatePort() const {
    return srcAggregatePort_;
  }
  /*
   * Get the VLAN on which this packet was received.
   */
  VlanID getSrcVlan() const {
    return srcVlan_.value();
  }
  /*
   * Get the VLAN (if any) on which this packet was received.
   *
   * TODO(skhare)
   * Once all the callsites for getSrcVlan() are converted to handle nullopt
   * return, remove the old definition of getSrcVlan() and rename this function
   * to getSrcVlan().
   */
  std::optional<VlanID> getSrcVlanIf() const {
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

  std::optional<uint8_t> cosQueue() const {
    return cosQueue_;
  }

  /*
   * Struct to hold reason information
   */
  struct RxReason {
    int bytes;
    std::string description;
  };

  /*
   * Return a vector of reasons for the packet
   */
  virtual std::vector<RxReason> getReasons() {
    return {};
  }

 protected:
  PortID srcPort_{0};
  bool isFromAggregatePort_{false};
  AggregatePortID srcAggregatePort_{0};
  // TODO(skhare)
  // srcVlan_ was originally not optional, and had default value of 0.
  // Several callsites rely on this default value being 0.
  // Once all those callsites are fixed, default to std::nullopt.
  std::optional<VlanID> srcVlan_{0};
  uint32_t len_{0};
  std::optional<uint8_t> cosQueue_;
};

} // namespace facebook::fboss
