// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>

#include "fboss/agent/NextHopResolver.h"
#include "fboss/agent/state/Mirror.h"

namespace facebook::fboss {

class Mirror;
struct MirrorTunnel;
class SwSwitch;

/**
 * MirrorManagerImpl handles the resolution of mirror destinations
 * to next hop ports and MAC addresses.
 *
 * This class uses NextHopResolver for common IP resolution logic
 * and adds mirror-specific functionality like tunnel building
 * and SFLOW eventor port handling.
 */
template <typename AddrT>
class MirrorManagerImpl {
 public:
  using NeighborEntryT = typename NextHopResolver<AddrT>::NeighborEntryT;

  explicit MirrorManagerImpl(SwSwitch* sw);
  ~MirrorManagerImpl() = default;

  std::shared_ptr<Mirror> updateMirror(const std::shared_ptr<Mirror>& mirror);

 private:
  MirrorTunnel resolveMirrorTunnel(
      const std::shared_ptr<SwitchState>& state,
      const AddrT& destinationIp,
      const std::optional<AddrT>& srcIp,
      const NextHop& nextHop,
      const std::shared_ptr<NeighborEntryT>& neighbor,
      const std::optional<TunnelUdpPorts>& udpPorts);

  PortID getEventorPortForSflowMirror(SwitchID switchId);

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
  NextHopResolver<AddrT> destinationAddrResolver_;
};

using MirrorManagerV4 = MirrorManagerImpl<folly::IPAddressV4>;
using MirrorManagerV6 = MirrorManagerImpl<folly::IPAddressV6>;

} // namespace facebook::fboss
