// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

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

USE_THRIFT_COW(IpTunnel);

class IpTunnel : public ThriftStructNode<IpTunnel, state::IpTunnelFields> {
 public:
  using Base = ThriftStructNode<IpTunnel, state::IpTunnelFields>;
  using LegacyFields = IpTunnelFields;
  explicit IpTunnel(const std::string& id) {
    set<switch_state_tags::ipTunnelId>(id);
  }
  std::string getID() const {
    return get<switch_state_tags::ipTunnelId>()->cref();
  }
  cfg::TunnelType getType() const {
    return static_cast<cfg::TunnelType>(get<switch_state_tags::type>()->cref());
  }
  void setType(cfg::TunnelType type) {
    set<switch_state_tags::type>(static_cast<int>(type));
  }
  InterfaceID getUnderlayIntfId() const {
    return InterfaceID(get<switch_state_tags::underlayIntfId>()->cref());
  }
  void setUnderlayIntfId(InterfaceID id) {
    set<switch_state_tags::underlayIntfId>(static_cast<int>(id));
  }
  cfg::IpTunnelMode getTTLMode() const {
    const auto& mode = cref<switch_state_tags::ttlMode>();
    CHECK(mode);
    return static_cast<cfg::IpTunnelMode>(mode->cref());
  }
  void setTTLMode(cfg::IpTunnelMode mode) {
    setMode<switch_state_tags::ttlMode>(mode);
  }
  cfg::IpTunnelMode getDscpMode() const {
    const auto& mode = cref<switch_state_tags::dscpMode>();
    CHECK(mode);
    return static_cast<cfg::IpTunnelMode>(mode->cref());
  }
  void setDscpMode(cfg::IpTunnelMode mode) {
    setMode<switch_state_tags::dscpMode>(mode);
  }
  cfg::IpTunnelMode getEcnMode() const {
    const auto& mode = cref<switch_state_tags::ecnMode>();
    CHECK(mode);
    return static_cast<cfg::IpTunnelMode>(mode->cref());
  }
  void setEcnMode(cfg::IpTunnelMode mode) {
    setMode<switch_state_tags::ecnMode>(mode);
  }
  cfg::TunnelTerminationType getTunnelTermType() const {
    return static_cast<cfg::TunnelTerminationType>(
        get<switch_state_tags::tunnelTermType>()->cref());
  }
  void setTunnelTermType(cfg::TunnelTerminationType type) {
    set<switch_state_tags::tunnelTermType>(static_cast<int>(type));
  }
  folly::IPAddress getDstIP() const {
    return folly::IPAddress(get<switch_state_tags::dstIp>()->cref());
  }
  void setDstIP(folly::IPAddress ip) {
    set<switch_state_tags::dstIp>(ip.str());
  }
  folly::IPAddress getSrcIP() const {
    return folly::IPAddress(get<switch_state_tags::srcIp>()->cref());
  }
  void setSrcIP(folly::IPAddress ip) {
    set<switch_state_tags::srcIp>(ip.str());
  }
  folly::IPAddress getDstIPMask() const {
    return folly::IPAddress(get<switch_state_tags::dstIpMask>()->cref());
  }
  void setDstIPMask(folly::IPAddress ip) {
    set<switch_state_tags::dstIpMask>(ip.str());
  }
  folly::IPAddress getSrcIPMask() const {
    return folly::IPAddress(get<switch_state_tags::srcIpMask>()->cref());
  }
  void setSrcIPMask(folly::IPAddress ip) {
    set<switch_state_tags::srcIpMask>(ip.str());
  }

  folly::dynamic toFollyDynamic() const override {
    auto fields = IpTunnelFields::fromThrift(toThrift());
    return fields.toFollyDynamic();
  }

  folly::dynamic toFollyDynamicLegacy() const {
    return toFollyDynamic();
  }

  static std::shared_ptr<IpTunnel> fromFollyDynamic(const folly::dynamic& dyn) {
    auto fields = IpTunnelFields::fromFollyDynamic(dyn);
    return std::make_shared<IpTunnel>(fields.toThrift());
  }

  static std::shared_ptr<IpTunnel> fromFollyDynamicLegacy(
      const folly::dynamic& dyn) {
    return fromFollyDynamic(dyn);
  }

  template <typename Tag>
  void setMode(cfg::IpTunnelMode mode) {
    set<Tag>(static_cast<int>(mode));
  }

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
