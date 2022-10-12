// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <optional>

namespace facebook::fboss {

struct IpTunnelFields
    : public ThriftyFields<IpTunnelFields, state::IpTunnelFields> {
  // TODO(yijunli): BetterThriftyFields?
  explicit IpTunnelFields(std::string id) {
    auto& data = writableData();
    *data.ipTunnelId() = id;
  }

  template <typename Fn>
  void forEachChild(Fn /*fn*/) {}

  state::IpTunnelFields toThrift() const override {
    return data();
  }
  static IpTunnelFields fromThrift(const state::IpTunnelFields& ipTunnelThrift);
};

class IpTunnel
    : public ThriftyBaseT<state::IpTunnelFields, IpTunnel, IpTunnelFields> {
 public:
  std::string getID() const {
    return *getFields()->data().ipTunnelId();
  }
  cfg::TunnelType getType() const {
    return static_cast<cfg::TunnelType>(*getFields()->data().type());
  }
  void setType(cfg::TunnelType type) {
    writableFields()->writableData().type() = static_cast<int32_t>(type);
  }
  InterfaceID getUnderlayIntfId() const {
    return InterfaceID(*getFields()->data().underlayIntfId());
  }
  void setUnderlayIntfId(InterfaceID id) {
    writableFields()->writableData().underlayIntfId() = id;
  }
  cfg::IpTunnelMode getTTLMode() const {
    return static_cast<cfg::IpTunnelMode>(*getFields()->data().ttlMode());
  }
  void setTTLMode(cfg::IpTunnelMode mode) {
    writableFields()->writableData().ttlMode() = static_cast<int32_t>(mode);
  }
  cfg::IpTunnelMode getDscpMode() const {
    return static_cast<cfg::IpTunnelMode>(*getFields()->data().dscpMode());
  }
  void setDscpMode(cfg::IpTunnelMode mode) {
    writableFields()->writableData().dscpMode() = static_cast<int32_t>(mode);
  }
  cfg::IpTunnelMode getEcnMode() const {
    return static_cast<cfg::IpTunnelMode>(*getFields()->data().ecnMode());
  }
  void setEcnMode(cfg::IpTunnelMode mode) {
    writableFields()->writableData().ecnMode() = static_cast<int32_t>(mode);
  }
  cfg::TunnelTerminationType getTunnelTermType() const {
    return static_cast<cfg::TunnelTerminationType>(
        *getFields()->data().tunnelTermType());
  }
  void setTunnelTermType(cfg::TunnelTerminationType type) {
    writableFields()->writableData().tunnelTermType() =
        static_cast<int32_t>(type);
  }
  folly::IPAddress getDstIP() const {
    return folly::IPAddress(*getFields()->data().dstIp());
  }
  void setDstIP(folly::IPAddress ip) {
    writableFields()->writableData().dstIp() = ip.str();
  }
  folly::IPAddress getSrcIP() const {
    return folly::IPAddress(*getFields()->data().srcIp());
  }
  void setSrcIP(folly::IPAddress ip) {
    writableFields()->writableData().srcIp() = ip.str();
  }
  folly::IPAddress getDstIPMask() const {
    return folly::IPAddress(*getFields()->data().dstIpMask());
  }
  void setDstIPMask(folly::IPAddress ip) {
    writableFields()->writableData().dstIpMask() = ip.str();
  }
  folly::IPAddress getSrcIPMask() const {
    return folly::IPAddress(*getFields()->data().srcIpMask());
  }
  void setSrcIPMask(folly::IPAddress ip) {
    writableFields()->writableData().srcIpMask() = ip.str();
  }

  bool operator==(const IpTunnel& ipTunnel) {
    return (getID() == ipTunnel.getID()) && (getType() == ipTunnel.getType()) &&
        (getUnderlayIntfId() == ipTunnel.getUnderlayIntfId()) &&
        (getTTLMode() == ipTunnel.getTTLMode()) &&
        (getDscpMode() == ipTunnel.getDscpMode()) &&
        (getEcnMode() == ipTunnel.getEcnMode()) &&
        (getDstIP() == ipTunnel.getDstIP()) &&
        (getSrcIP() == ipTunnel.getSrcIP()) &&
        (getDstIPMask() == ipTunnel.getDstIPMask()) &&
        (getSrcIPMask() == ipTunnel.getSrcIPMask()) &&
        (getTunnelTermType() == ipTunnel.getTunnelTermType());
  }
  bool operator!=(const IpTunnel& ipTunnel) {
    return !(*this == ipTunnel);
  }

 private:
  // Inherit the constructors required for clone()
  using ThriftyBaseT<state::IpTunnelFields, IpTunnel, IpTunnelFields>::
      ThriftyBaseT;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
