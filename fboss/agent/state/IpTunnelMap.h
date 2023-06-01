// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>
#pragma once

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/IpTunnel.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwitchState;

using IpTunnelMapLegacyTraits = NodeMapTraits<std::string, IpTunnel>;

using IpTunnelMapClass = apache::thrift::type_class::map<
    apache::thrift::type_class::string,
    apache::thrift::type_class::structure>;
using IpTunnelMapThriftType = std::map<std::string, state::IpTunnelFields>;

class IpTunnelMap;
using IpTunnelMapTraits = ThriftMapNodeTraits<
    IpTunnelMap,
    IpTunnelMapClass,
    IpTunnelMapThriftType,
    IpTunnel>;
/*
 * A container for all the present IpTunnels
 */
class IpTunnelMap : public ThriftMapNode<IpTunnelMap, IpTunnelMapTraits> {
 public:
  using Base = ThriftMapNode<IpTunnelMap, IpTunnelMapTraits>;
  using Traits = IpTunnelMapTraits;
  IpTunnelMap();
  ~IpTunnelMap() override;

  const std::shared_ptr<IpTunnel> getTunnel(std::string id) const {
    return getNode(id);
  }
  std::shared_ptr<IpTunnel> getTunnelIf(std::string id) const {
    return getNodeIf(id);
  }

  void addTunnel(const std::shared_ptr<IpTunnel>& Tunnel);
  void updateTunnel(const std::shared_ptr<IpTunnel>& Tunnel);
  void removeTunnel(std::string id);

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

using MultiSwitchIpTunnelMapTypeClass = apache::thrift::type_class::
    map<apache::thrift::type_class::string, IpTunnelMapClass>;
using MultiSwitchIpTunnelMapThriftType =
    std::map<std::string, IpTunnelMapThriftType>;

class MultiSwitchIpTunnelMap;

using MultiSwitchIpTunnelMapTraits = ThriftMultiSwitchMapNodeTraits<
    MultiSwitchIpTunnelMap,
    MultiSwitchIpTunnelMapTypeClass,
    MultiSwitchIpTunnelMapThriftType,
    IpTunnelMap>;

class HwSwitchMatcher;

class MultiSwitchIpTunnelMap : public ThriftMultiSwitchMapNode<
                                   MultiSwitchIpTunnelMap,
                                   MultiSwitchIpTunnelMapTraits> {
 public:
  using Traits = MultiSwitchIpTunnelMapTraits;
  using BaseT = ThriftMultiSwitchMapNode<
      MultiSwitchIpTunnelMap,
      MultiSwitchIpTunnelMapTraits>;
  using BaseT::modify;

  MultiSwitchIpTunnelMap() = default;
  virtual ~MultiSwitchIpTunnelMap() = default;

  MultiSwitchIpTunnelMap* modify(std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
