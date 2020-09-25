// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <boost/container/flat_set.hpp>
#include <list>

#include <folly/IPAddress.h>
#include <optional>
#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

using boost::container::flat_set;

struct TunnelUdpPorts {
  uint32_t udpSrcPort;
  uint32_t udpDstPort;
  TunnelUdpPorts(uint32_t src, uint32_t dst)
      : udpSrcPort(src), udpDstPort(dst) {}
  bool operator==(const TunnelUdpPorts& that) const {
    return udpSrcPort == that.udpSrcPort && udpDstPort == that.udpDstPort;
  }
  bool operator<(const TunnelUdpPorts& that) const {
    return std::tie(udpSrcPort, udpDstPort) <
        std::tie(that.udpSrcPort, that.udpDstPort);
  }
};

struct MirrorTunnel {
  folly::IPAddress srcIp, dstIp;
  folly::MacAddress srcMac, dstMac;
  std::optional<TunnelUdpPorts> udpPorts;
  uint8_t ttl;
  uint16_t greProtocol;
  static constexpr auto kTTL = 255;
  static constexpr auto kGreProto = 0x88be;

  MirrorTunnel(
      const folly::IPAddress& srcIp,
      const folly::IPAddress& dstIp,
      const folly::MacAddress& srcMac,
      const folly::MacAddress& dstMac,
      uint8_t ttl = kTTL)
      : srcIp(srcIp),
        dstIp(dstIp),
        srcMac(srcMac),
        dstMac(dstMac),
        udpPorts(std::nullopt),
        ttl(ttl),
        greProtocol(kGreProto) {}

  MirrorTunnel(
      const folly::IPAddress& srcIp,
      const folly::IPAddress& dstIp,
      const folly::MacAddress& srcMac,
      const folly::MacAddress& dstMac,
      const TunnelUdpPorts& sflowPorts,
      uint8_t ttl = kTTL)
      : srcIp(srcIp),
        dstIp(dstIp),
        srcMac(srcMac),
        dstMac(dstMac),
        udpPorts(sflowPorts),
        ttl(ttl),
        greProtocol(0) {}

  bool operator==(const MirrorTunnel& rhs) const {
    return srcIp == rhs.srcIp && dstIp == rhs.dstIp && srcMac == rhs.srcMac &&
        dstMac == rhs.dstMac && udpPorts == rhs.udpPorts && ttl == rhs.ttl &&
        greProtocol == rhs.greProtocol;
  }

  bool operator<(const MirrorTunnel& rhs) const {
    return std::tie(srcIp, dstIp, srcMac, dstMac, udpPorts, ttl, greProtocol) <
        std::tie(
               rhs.srcIp,
               rhs.dstIp,
               rhs.srcMac,
               rhs.dstMac,
               rhs.udpPorts,
               rhs.ttl,
               rhs.greProtocol);
  }

  folly::dynamic toFollyDynamic() const;

  static MirrorTunnel fromFollyDynamic(const folly::dynamic& json);
};

struct MirrorFields {
  MirrorFields(
      const std::string& name,
      const std::optional<PortID>& egressPort,
      const std::optional<folly::IPAddress>& destinationIp,
      const std::optional<folly::IPAddress>& srcIp = std::nullopt,
      const std::optional<TunnelUdpPorts>& udpPorts = std::nullopt,
      const uint8_t& dscp = cfg::switch_config_constants::DEFAULT_MIRROR_DSCP_,
      const bool truncate = false)
      : name(name),
        egressPort(egressPort),
        destinationIp(destinationIp),
        srcIp(srcIp),
        udpPorts(udpPorts),
        dscp(dscp),
        truncate(truncate) {
    if (egressPort.has_value()) {
      configHasEgressPort = true;
    }
  }

  template <typename Fn>
  void forEachChild(Fn /* unused */) {}

  std::string name;
  std::optional<PortID> egressPort;
  std::optional<folly::IPAddress> destinationIp;
  std::optional<folly::IPAddress> srcIp;
  std::optional<TunnelUdpPorts> udpPorts;
  uint8_t dscp;
  bool truncate;
  std::optional<MirrorTunnel> resolvedTunnel;
  bool configHasEgressPort{false};

  folly::dynamic toFollyDynamic() const;
};

class Mirror : public NodeBaseT<Mirror, MirrorFields> {
 public:
  enum Type { SPAN = 1, ERSPAN = 2, SFLOW = 3 };
  Mirror(
      std::string name,
      std::optional<PortID> egressPort,
      std::optional<folly::IPAddress> destinationIp,
      std::optional<folly::IPAddress> srcIp = std::nullopt,
      std::optional<TunnelUdpPorts> udpPorts = std::nullopt,
      uint8_t dscp = cfg::switch_config_constants::DEFAULT_MIRROR_DSCP_,
      bool truncate = false);
  std::string getID() const;
  std::optional<PortID> getEgressPort() const;
  std::optional<folly::IPAddress> getDestinationIp() const;
  std::optional<folly::IPAddress> getSrcIp() const;
  std::optional<TunnelUdpPorts> getTunnelUdpPorts() const;
  std::optional<MirrorTunnel> getMirrorTunnel() const;
  uint8_t getDscp() const;
  bool getTruncate() const;
  void setTruncate(bool truncate);
  void setEgressPort(PortID egressPort);
  void setMirrorTunnel(const MirrorTunnel& tunnel);
  bool configHasEgressPort() const;
  bool isResolved() const;

  static std::shared_ptr<Mirror> fromFollyDynamic(const folly::dynamic& json);
  folly::dynamic toFollyDynamic() const override;

  bool operator==(const Mirror& rhs) const;
  bool operator!=(const Mirror& rhs) const;

  Type type() const;

 private:
  // Inherit the constructors required for clone()

  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
