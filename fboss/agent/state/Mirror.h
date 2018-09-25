// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <boost/container/flat_set.hpp>
#include <list>

#include <folly/IPAddress.h>
#include <folly/Optional.h>
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/types.h"


namespace facebook {
namespace fboss {

using boost::container::flat_set;

struct MirrorTunnel {
  folly::IPAddress srcIp, dstIp;
  folly::MacAddress srcMac, dstMac;
  uint8_t ttl;
  uint16_t greProtocol;
  NextHop nextHop;

  MirrorTunnel()
      : srcMac(folly::MacAddress::ZERO),
        dstMac(folly::MacAddress::ZERO),
        ttl(255),
        greProtocol(0x88be) {}

  bool operator==(const MirrorTunnel& rhs) const {
    return srcIp == rhs.srcIp && dstIp == rhs.dstIp && srcMac == rhs.srcMac &&
        dstMac == rhs.dstMac && ttl == rhs.ttl &&
        greProtocol == rhs.greProtocol && nextHop == rhs.nextHop;
  }
};

struct MirrorFields {
  MirrorFields(
      std::string name,
      folly::Optional<PortID> mirrorEgressPort,
      folly::Optional<folly::IPAddress> tunnelDestinationIp)
      : name_(name),
        egressPort_(mirrorEgressPort),
        tunnelDestinationIp_(tunnelDestinationIp) {
    if (tunnelDestinationIp_.hasValue() && !egressPort_.hasValue()) {
      configHasEgressPort_ = true;
    }
  }

  template <typename Fn>
  void forEachChild(Fn /* unused */) {}

  std::string name_;
  folly::Optional<PortID> egressPort_;
  folly::Optional<folly::IPAddress> tunnelDestinationIp_;
  folly::Optional<MirrorTunnel> resolvedTunnel_;
  bool configHasEgressPort_{false};
};

class Mirror : public NodeBaseT<Mirror, MirrorFields> {
 public:
  Mirror(
      std::string name,
      folly::Optional<PortID> mirrorEgressPort,
      folly::Optional<folly::IPAddress> tunnelDestinationIp);
  std::string getID() const;
  folly::Optional<PortID> getMirrorEgressPort() const;
  folly::Optional<folly::IPAddress> getMirrorTunnelDestinationIp() const;
  folly::Optional<MirrorTunnel> getMirrorTunnel() const;
  void setMirrorEgressPort(PortID egressPort);
  void setMirrorTunnel(MirrorTunnel tunnel);
  const flat_set<PortID>& getMirrorPortsRef() const;
  const flat_set<std::shared_ptr<AclEntry>>& getFlowsRef() const;
  bool configHasEgressPort() const;
  bool isResolved() const;

  static std::shared_ptr<Mirror> fromFollyDynamic(const folly::dynamic& json);
  folly::dynamic toFollyDynamic() const override;

  bool operator==(const Mirror& rhs) const;
  bool operator!=(const Mirror& rhs) const;

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

} // namespace fboss
} // namespace facebook
