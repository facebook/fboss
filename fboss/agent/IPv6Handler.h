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

#include "fboss/agent/StateObserver.h"
#include "fboss/agent/ndp/IPv6RouteAdvertiser.h"
#include "fboss/agent/packet/ICMPHdr.h"
#include "fboss/agent/packet/NDP.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_map.hpp>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <memory>
namespace folly {
namespace io {
class Cursor;
}
} // namespace folly

namespace facebook::fboss {

class IPv6Hdr;
class Interface;
class RxPacket;
class StateDelta;
class SwitchState;
class Vlan;
class SwSwitch;

class IPv6Handler : public StateObserver {
 public:
  enum : uint16_t { ETHERTYPE_IPV6 = 0x86dd };
  enum : uint32_t { IPV6_MIN_MTU = 1280 };

  explicit IPv6Handler(SwSwitch* sw);
  ~IPv6Handler() override;

  void stateUpdated(const StateDelta& delta) override;

  void handlePacket(
      std::unique_ptr<RxPacket> pkt,
      folly::MacAddress dst,
      folly::MacAddress src,
      folly::io::Cursor cursor);

  void floodNeighborAdvertisements();
  void sendNeighborSolicitation(
      const folly::IPAddressV6& targetIP,
      const std::shared_ptr<Vlan> vlan);

  /*
   * TODO(aeckert): 17949183 unify packet handling pipeline and then
   * make this private again.
   */
  void sendMulticastNeighborSolicitations(
      PortID ingressPort,
      const folly::IPAddressV6& targetIP);

  /*
   * Only tests use this API directly, otherwise should only be invoked
   * internally in response to incoming packets
   */
  void sendNeighborAdvertisement(
      std::optional<VlanID> vlan,
      folly::MacAddress srcMac,
      folly::IPAddressV6 srcIP,
      folly::MacAddress dstMac,
      folly::IPAddressV6 dstIP,
      const std::optional<PortDescriptor>& portDescriptor =
          std::optional<PortDescriptor>());

  /*
   * These two static methods are for sending out an NDP solicitation.
   * The second version actually calls the first and is there
   * for the convenience of the caller. The first version
   * does not access the SwitchState so should be preferred where
   * possible.
   */
  static void sendMulticastNeighborSolicitation(
      SwSwitch* sw,
      const folly::IPAddressV6& targetIP,
      const folly::MacAddress& srcMac,
      const std::optional<VlanID>& vlanID);
  static void sendMulticastNeighborSolicitation(
      SwSwitch* sw,
      const folly::IPAddressV6& targetIP,
      const std::shared_ptr<Vlan>& vlan);

  static void sendUnicastNeighborSolicitation(
      SwSwitch* sw,
      const folly::IPAddressV6& targetIP,
      const folly::MacAddress& targetMac,
      const folly::IPAddressV6& srcIP,
      const folly::MacAddress& srcMac,
      const VlanID& vlanID,
      const PortDescriptor& portDescriptor);

 private:
  struct ICMPHeaders;
  typedef boost::container::flat_map<InterfaceID, IPv6RouteAdvertiser> RAMap;

  // Forbidden copy constructor and assignment operator
  IPv6Handler(IPv6Handler const&) = delete;
  IPv6Handler& operator=(IPv6Handler const&) = delete;

  bool raEnabled(const Interface* intf) const;
  void intfAdded(const SwitchState* state, const Interface* intf);
  void intfDeleted(const Interface* intf);

  void sendICMPv6TimeExceeded(
      VlanID srcVlan,
      folly::MacAddress dst,
      folly::MacAddress src,
      IPv6Hdr& v6Hdr,
      folly::io::Cursor cursor);

  void sendICMPv6PacketTooBig(
      PortID srcPort,
      VlanID srcVlan,
      folly::MacAddress dst,
      folly::MacAddress src,
      IPv6Hdr& v6Hdr,
      int expectedMtu,
      folly::io::Cursor cursor);
  /**
   * Function to handle ICMPv6
   *
   * For now, the controller just handle ND related ICMPv6 packets. For
   * other types, the packet is returned back to the caller through the return
   * value.
   *
   * @return nullptr The packet has been handled by this function
   *         others The function does not handle the packet. It is up to the
   *                caller to decide what to do with the packet (i.e. forward
   *                the packet to host)
   */
  std::unique_ptr<RxPacket> handleICMPv6Packet(
      std::unique_ptr<RxPacket> pkt,
      folly::MacAddress dst,
      folly::MacAddress src,
      const IPv6Hdr& ipv6,
      folly::io::Cursor cursor);
  void handleRouterSolicitation(
      std::unique_ptr<RxPacket> pkt,
      const ICMPHeaders& hdr,
      folly::io::Cursor cursor);
  void handleRouterAdvertisement(
      std::unique_ptr<RxPacket> pkt,
      const ICMPHeaders& hdr,
      folly::io::Cursor cursor);
  void handleNeighborSolicitation(
      std::unique_ptr<RxPacket> pkt,
      const ICMPHeaders& hdr,
      folly::io::Cursor cursor);
  void handleNeighborAdvertisement(
      std::unique_ptr<RxPacket> pkt,
      const ICMPHeaders& hdr,
      folly::io::Cursor cursor);

  bool checkNdpPacket(const ICMPHeaders& hdr, const RxPacket* pkt) const;

  void resolveDestAndHandlePacket(
      IPv6Hdr hdr,
      std::unique_ptr<RxPacket> pkt,
      folly::MacAddress dst,
      folly::MacAddress src,
      folly::io::Cursor cursor);

  static void sendNeighborSolicitation(
      SwSwitch* sw,
      const folly::IPAddressV6& dstIP,
      const folly::MacAddress& dstMac,
      const folly::IPAddressV6& srcIP,
      const folly::MacAddress& srcMac,
      const folly::IPAddressV6& neighborIP,
      const std::optional<VlanID>& vlanID,
      const std::optional<PortDescriptor>& portDescriptor =
          std::optional<PortDescriptor>(),
      const NDPOptions& options = NDPOptions());

  SwSwitch* sw_{nullptr};
  RAMap routeAdvertisers_;
};

} // namespace facebook::fboss
