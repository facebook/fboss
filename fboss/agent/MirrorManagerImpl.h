// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>

#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/NdpEntry.h"
#include "fboss/agent/state/RouteNextHopEntry.h"

namespace facebook {
namespace fboss {

class Mirror;
class MirrorTunnel;
class SwSwitch;

template <typename AddrT>
class MirrorManagerImpl {
 public:
  using NeighborEntryT = std::conditional_t<
      std::is_same<AddrT, folly::IPAddressV4>::value,
      ArpEntry,
      NdpEntry>;
  using NextHopSet = RouteNextHopEntry::NextHopSet;

  explicit MirrorManagerImpl(SwSwitch* sw) : sw_(sw) {}
  ~MirrorManagerImpl() {}

  std::shared_ptr<Mirror> updateMirror(const std::shared_ptr<Mirror>& mirror);

private:
 NextHopSet resolveMirrorNextHops(
     const std::shared_ptr<SwitchState>& state,
     const AddrT& destinationIp);

 std::shared_ptr<NeighborEntryT> resolveMirrorNextHopNeighbor(
     const std::shared_ptr<SwitchState>& state,
     const std::shared_ptr<Mirror>& mirror,
     const AddrT& destinationIp,
     const NextHop& nexthop) const;

 MirrorTunnel resolveMirrorTunnel(
     const std::shared_ptr<SwitchState>& state,
     const AddrT& destinationIp,
     const NextHop& nextHop,
     const std::shared_ptr<NeighborEntryT>& neighbor);

 template <typename ADDRT = AddrT>
 typename std::
     enable_if<std::is_same<ADDRT, folly::IPAddressV4>::value, ADDRT>::type
     getIPAddress(const folly::IPAddress& ip) const {
   return ip.asV4();
 }

 template <typename ADDRT = AddrT>
 typename std::
     enable_if<std::is_same<ADDRT, folly::IPAddressV6>::value, ADDRT>::type
     getIPAddress(const folly::IPAddress& ip) const {
   return ip.asV6();
 }

  SwSwitch* sw_;
};

using MirrorManagerV4 = MirrorManagerImpl<folly::IPAddressV4>;
using MirrorManagerV6 = MirrorManagerImpl<folly::IPAddressV6>;

} // namespace fboss
} // namespace facebook
