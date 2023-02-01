// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <boost/container/flat_set.hpp>
#include <list>

#include <folly/IPAddress.h>
#include <memory>
#include <optional>
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/types.h"
#include "folly/MacAddress.h"

namespace facebook::fboss {

using boost::container::flat_set;

enum class MirrorDirection { INGRESS = 1, EGRESS = 2 };
enum class MirrorAction { START = 1, STOP = 2 };

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

  state::MirrorTunnel toThrift() const {
    state::MirrorTunnel tunnel{};

    tunnel.srcIp() = network::toBinaryAddress(srcIp);
    tunnel.dstIp() = network::toBinaryAddress(dstIp);
    tunnel.srcMac() = srcMac.toString();
    tunnel.dstMac() = dstMac.toString();
    if (udpPorts) {
      tunnel.udpSrcPort() = udpPorts->udpSrcPort;
      tunnel.udpDstPort() = udpPorts->udpDstPort;
    }
    tunnel.ttl() = ttl;
    return tunnel;
  }

  static MirrorTunnel fromThrift(const state::MirrorTunnel& tunnel) {
    auto srcL4Port = tunnel.udpSrcPort();
    auto dstL4Port = tunnel.udpDstPort();
    if (srcL4Port && dstL4Port) {
      return MirrorTunnel(
          network::toIPAddress(*tunnel.srcIp()),
          network::toIPAddress(*tunnel.dstIp()),
          folly::MacAddress(*tunnel.srcMac()),
          folly::MacAddress(*tunnel.dstMac()),
          TunnelUdpPorts(*srcL4Port, *dstL4Port),
          *tunnel.ttl()

      );
    }
    return MirrorTunnel(
        network::toIPAddress(*tunnel.srcIp()),
        network::toIPAddress(*tunnel.dstIp()),
        folly::MacAddress(*tunnel.srcMac()),
        folly::MacAddress(*tunnel.dstMac()),
        *tunnel.ttl());
  }

  folly::dynamic toFollyDynamic() const;

  static MirrorTunnel fromFollyDynamic(const folly::dynamic& json);
};

struct MirrorFields : public ThriftyFields<MirrorFields, state::MirrorFields> {
  MirrorFields(
      const std::string& name,
      const std::optional<PortID>& egressPort,
      const std::optional<folly::IPAddress>& destinationIp,
      const std::optional<folly::IPAddress>& srcIp = std::nullopt,
      const std::optional<TunnelUdpPorts>& udpPorts = std::nullopt,
      const uint8_t& dscp = cfg::switch_config_constants::DEFAULT_MIRROR_DSCP_,
      const bool truncate = false) {
    auto& data = writableData();
    data.name() = name;
    if (egressPort) {
      data.egressPort() = *egressPort;
      data.configHasEgressPort() = true;
    }
    if (destinationIp) {
      data.destinationIp() = network::toBinaryAddress(*destinationIp);
    } else {
      // SPAN mirror always resolved.
      data.isResolved() = true;
    }
    if (srcIp) {
      data.srcIp() = network::toBinaryAddress(*srcIp);
    }
    if (udpPorts) {
      data.udpSrcPort() = udpPorts->udpSrcPort;
      data.udpDstPort() = udpPorts->udpDstPort;
    }
    data.dscp() = dscp;
    data.truncate() = truncate;
  }

  explicit MirrorFields(const state::MirrorFields& data) {
    writableData() = data;
  }

  bool operator==(const MirrorFields& other) const {
    return (
        name() == other.name() && egressPort() == other.egressPort() &&
        destinationIp() == other.destinationIp() && srcIp() == other.srcIp() &&
        udpPorts() == other.udpPorts() && dscp() == other.dscp() &&
        truncate() == other.truncate() &&
        resolvedTunnel() == other.resolvedTunnel() &&
        configHasEgressPort() == other.configHasEgressPort());
  }

  template <typename Fn>
  void forEachChild(Fn /* unused */) {}

  std::string name() const {
    return *data().name();
  }
  std::optional<PortID> egressPort() const {
    if (auto port = data().egressPort()) {
      return PortID(*port);
    }
    return std::nullopt;
  }
  std::optional<folly::IPAddress> destinationIp() const {
    if (auto ip = data().destinationIp()) {
      return network::toIPAddress(*ip);
    }
    return std::nullopt;
  }
  std::optional<folly::IPAddress> srcIp() const {
    if (auto ip = data().srcIp()) {
      return network::toIPAddress(*ip);
    }
    return std::nullopt;
  }
  std::optional<TunnelUdpPorts> udpPorts() const {
    auto srcL4Port = data().udpSrcPort();
    auto dstL4Port = data().udpDstPort();
    if (srcL4Port && dstL4Port) {
      return TunnelUdpPorts(*srcL4Port, *dstL4Port);
    }
    return std::nullopt;
  }
  uint8_t dscp() const {
    return *data().dscp();
  }
  bool truncate() const {
    return *data().truncate();
  }
  std::optional<MirrorTunnel> resolvedTunnel() const {
    auto tunnel = data().tunnel();
    if (!tunnel) {
      return std::nullopt;
    }
    return MirrorTunnel::fromThrift(*tunnel);
  }
  bool configHasEgressPort() const {
    return *data().configHasEgressPort();
  }

  folly::dynamic toFollyDynamicLegacy() const;
  static MirrorFields fromFollyDynamicLegacy(const folly::dynamic& dyn);
  state::MirrorFields toThrift() const override;
  static MirrorFields fromThrift(state::MirrorFields const& fields);
  static folly::dynamic migrateToThrifty(folly::dynamic const& dyn);
  static void migrateFromThrifty(folly::dynamic& dyn);
};

USE_THRIFT_COW(Mirror);

class Mirror : public ThriftStructNode<Mirror, state::MirrorFields> {
 public:
  using BaseT = ThriftStructNode<Mirror, state::MirrorFields>;
  Mirror(
      std::string name,
      std::optional<PortID> egressPort,
      std::optional<folly::IPAddress> destinationIp,
      std::optional<folly::IPAddress> srcIp = std::nullopt,
      std::optional<TunnelUdpPorts> udpPorts = std::nullopt,
      uint8_t dscp = cfg::switch_config_constants::DEFAULT_MIRROR_DSCP_,
      bool truncate = false);
  enum Type { SPAN = 1, ERSPAN = 2, SFLOW = 3 };
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

  static std::shared_ptr<Mirror> fromFollyDynamicLegacy(
      const folly::dynamic& json);
  folly::dynamic toFollyDynamicLegacy() const;

  static std::shared_ptr<Mirror> fromFollyDynamic(const folly::dynamic& json) {
    // for backward compatibility until warm boot moves to thrift switch state
    return fromFollyDynamicLegacy(json);
  }
  folly::dynamic toFollyDynamic() const override {
    // for backward compatibility until warm boot moves to thrift switch state
    return toFollyDynamicLegacy();
  }

  Type type() const;

  // TODO(pshaikh): make this private
  void markResolved();

 private:
  // Inherit the constructors required for clone()

  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
