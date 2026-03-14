// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

#include <optional>

namespace facebook::fboss {

USE_THRIFT_COW(Srv6Tunnel);

class Srv6Tunnel
    : public ThriftStructNode<Srv6Tunnel, state::Srv6TunnelFields> {
 public:
  using Base = ThriftStructNode<Srv6Tunnel, state::Srv6TunnelFields>;
  explicit Srv6Tunnel(const std::string& id) {
    set<switch_state_tags::srv6TunnelId>(id);
  }
  std::string getID() const {
    return get<switch_state_tags::srv6TunnelId>()->cref();
  }
  InterfaceID getUnderlayIntfId() const {
    return InterfaceID(get<switch_state_tags::underlayIntfId>()->cref());
  }
  void setUnderlayIntfId(InterfaceID id) {
    set<switch_state_tags::underlayIntfId>(static_cast<int>(id));
  }
  std::optional<folly::IPAddress> getSrcIP() const {
    const auto& ip = cref<switch_state_tags::srcIp>();
    if (!ip) {
      return std::nullopt;
    }
    return folly::IPAddress(ip->cref());
  }
  void setSrcIP(std::optional<folly::IPAddress> ip) {
    if (!ip) {
      ref<switch_state_tags::srcIp>().reset();
      return;
    }
    set<switch_state_tags::srcIp>(ip->str());
  }
  std::optional<folly::IPAddress> getDstIP() const {
    const auto& ip = cref<switch_state_tags::dstIp>();
    if (!ip) {
      return std::nullopt;
    }
    return folly::IPAddress(ip->cref());
  }
  void setDstIP(std::optional<folly::IPAddress> ip) {
    if (!ip) {
      ref<switch_state_tags::dstIp>().reset();
      return;
    }
    set<switch_state_tags::dstIp>(ip->str());
  }
  std::optional<cfg::TunnelMode> getTTLMode() const {
    const auto& mode = cref<switch_state_tags::ttlMode>();
    if (!mode) {
      return std::nullopt;
    }
    return mode->cref();
  }
  void setTTLMode(std::optional<cfg::TunnelMode> mode) {
    if (!mode) {
      ref<switch_state_tags::ttlMode>().reset();
      return;
    }
    set<switch_state_tags::ttlMode>(*mode);
  }
  std::optional<cfg::TunnelMode> getDscpMode() const {
    const auto& mode = cref<switch_state_tags::dscpMode>();
    if (!mode) {
      return std::nullopt;
    }
    return mode->cref();
  }
  void setDscpMode(std::optional<cfg::TunnelMode> mode) {
    if (!mode) {
      ref<switch_state_tags::dscpMode>().reset();
      return;
    }
    set<switch_state_tags::dscpMode>(*mode);
  }
  std::optional<cfg::TunnelMode> getEcnMode() const {
    const auto& mode = cref<switch_state_tags::ecnMode>();
    if (!mode) {
      return std::nullopt;
    }
    return mode->cref();
  }
  void setEcnMode(std::optional<cfg::TunnelMode> mode) {
    if (!mode) {
      ref<switch_state_tags::ecnMode>().reset();
      return;
    }
    set<switch_state_tags::ecnMode>(*mode);
  }
  std::optional<cfg::TunnelTerminationType> getTunnelTermType() const {
    const auto& type = cref<switch_state_tags::tunnelTermType>();
    if (!type) {
      return std::nullopt;
    }
    return type->cref();
  }
  void setTunnelTermType(std::optional<cfg::TunnelTerminationType> type) {
    if (!type) {
      ref<switch_state_tags::tunnelTermType>().reset();
      return;
    }
    set<switch_state_tags::tunnelTermType>(*type);
  }
  TunnelType getType() const {
    return get<switch_state_tags::tunnelType>()->cref();
  }
  void setType(TunnelType type) {
    set<switch_state_tags::tunnelType>(type);
  }

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
