// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <optional>

namespace facebook::fboss {

enum TunnelType {
  IPINIP,
};

enum TunnelTermType {
  MP2MP,
};

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
  TunnelType getType() const {
    return static_cast<TunnelType>(*getFields()->data().type());
  }
  void setType(TunnelType type) {
    writableFields()->writableData().type() = type;
  }
  InterfaceID getUnderlayIntfId() const {
    return InterfaceID(*getFields()->data().underlayIntfId());
  }
  void setUnderlayIntfId(InterfaceID id) {
    writableFields()->writableData().underlayIntfId() = id;
  }
  int32_t getTTLMode() const {
    return *getFields()->data().ttlMode();
  }
  void setTTLMode(int32_t mode) {
    writableFields()->writableData().ttlMode() = mode;
  }
  int32_t getDscpMode() const {
    return *getFields()->data().dscpMode();
  }
  void setDscpMode(int32_t mode) {
    writableFields()->writableData().dscpMode() = mode;
  }
  int32_t getEcnMode() const {
    return *getFields()->data().ecnMode();
  }
  void setEcnMode(int32_t mode) {
    writableFields()->writableData().ecnMode() = mode;
  }
  TunnelTermType getTunnelTermType() const {
    return static_cast<TunnelTermType>(*getFields()->data().tunnelTermType());
  }
  void setTunnelTermType(TunnelTermType type) {
    writableFields()->writableData().tunnelTermType() = type;
  }
  // TODO: move the type conversion to elsewhere?
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
