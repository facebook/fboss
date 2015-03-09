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

#include "fboss/agent/types.h"
#include "fboss/agent/state/NeighborEntry.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/Vlan.h"

#include <memory>

#include <folly/IPAddressV4.h>
#include <folly/MacAddress.h>

namespace folly { namespace io {
class Cursor;
}}

namespace facebook { namespace fboss {

class RxPacket;
class SwSwitch;
class SwitchState;
class Vlan;

class ArpHandler {
 public:
  enum : uint16_t { ETHERTYPE_ARP = 0x0806 };

  explicit ArpHandler(SwSwitch* sw);

  void handlePacket(std::unique_ptr<RxPacket> pkt,
                    folly::MacAddress dst,
                    folly::MacAddress src,
                    folly::io::Cursor cursor);
  void sendArpRequest(std::shared_ptr<Vlan> vlan,
                      std::shared_ptr<Interface> intf,
                      folly::IPAddressV4 senderIP,
                      folly::IPAddressV4 targetIP);

  /*
   * Send gratuitous arp on all vlans
   * */
  void floodGratuituousArp();
  uint32_t flushArpEntryBlocking(folly::IPAddressV4, VlanID vlan);

 private:
  // Forbidden copy constructor and assignment operator
  ArpHandler(ArpHandler const &) = delete;
  ArpHandler& operator=(ArpHandler const &) = delete;

  void sendArpReply(VlanID vlan, PortID port,
                    folly::MacAddress senderMac,
                    folly::IPAddressV4 senderIP,
                    folly::MacAddress targetMac,
                    folly::IPAddressV4 targetIP);
  void updateExistingArpEntry(const std::shared_ptr<Vlan>& vlan,
                              folly::IPAddressV4 ip,
                              folly::MacAddress mac,
                              PortID port);
  void updateArpEntry(const std::shared_ptr<Vlan>& vlan,
                      folly::IPAddressV4 ip,
                      folly::MacAddress mac,
                      PortID port,
                      InterfaceID intfID);
  void setPendingArpEntry(InterfaceID intf, std::shared_ptr<Vlan> vlan,
                          folly::IPAddressV4 ip);
  void arpUpdateRequired(VlanID vlanID,
                         folly::IPAddressV4 ip,
                         folly::MacAddress mac,
                         PortID port,
                         InterfaceID intfID,
                         bool addNewEntry);

  std::shared_ptr<SwitchState> performNeighborFlush(
      const std::shared_ptr<SwitchState>& state,
      VlanID vlan,
      folly::IPAddressV4 ip,
      uint32_t* countFlushed);
  bool performNeighborFlush(std::shared_ptr<SwitchState>* state,
                            Vlan* vlan,
                            folly::IPAddressV4 ip);

  SwSwitch* sw_{nullptr};
};

}} // facebook::fboss
