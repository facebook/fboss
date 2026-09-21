// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>
#pragma once

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/Srv6Tunnel.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwitchState;

using Srv6TunnelMapLegacyTraits = NodeMapTraits<std::string, Srv6Tunnel>;

using Srv6TunnelMapClass = apache::thrift::type_class::map<
    apache::thrift::type_class::string,
    apache::thrift::type_class::structure>;
using Srv6TunnelMapThriftType = std::map<std::string, state::Srv6TunnelFields>;

class Srv6TunnelMap;
using Srv6TunnelMapTraits = ThriftMapNodeTraits<
    Srv6TunnelMap,
    Srv6TunnelMapClass,
    Srv6TunnelMapThriftType,
    Srv6Tunnel>;
/*
 * A container for all the present Srv6Tunnels
 */
class Srv6TunnelMap : public ThriftMapNode<Srv6TunnelMap, Srv6TunnelMapTraits> {
 public:
  using Base = ThriftMapNode<Srv6TunnelMap, Srv6TunnelMapTraits>;
  using Traits = Srv6TunnelMapTraits;
  Srv6TunnelMap();
  ~Srv6TunnelMap() override;

  const std::shared_ptr<Srv6Tunnel> getTunnel(std::string id) const {
    return getNode(id);
  }
  std::shared_ptr<Srv6Tunnel> getTunnelIf(std::string id) const {
    return getNodeIf(id);
  }

  void addTunnel(const std::shared_ptr<Srv6Tunnel>& tunnel);
  void updateTunnel(const std::shared_ptr<Srv6Tunnel>& tunnel);
  void removeTunnel(std::string id);

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

using MultiSwitchSrv6TunnelMapTypeClass = apache::thrift::type_class::
    map<apache::thrift::type_class::string, Srv6TunnelMapClass>;
using MultiSwitchSrv6TunnelMapThriftType =
    std::map<std::string, Srv6TunnelMapThriftType>;

class MultiSwitchSrv6TunnelMap;

using MultiSwitchSrv6TunnelMapTraits = ThriftMultiSwitchMapNodeTraits<
    MultiSwitchSrv6TunnelMap,
    MultiSwitchSrv6TunnelMapTypeClass,
    MultiSwitchSrv6TunnelMapThriftType,
    Srv6TunnelMap>;

class HwSwitchMatcher;

class MultiSwitchSrv6TunnelMap : public ThriftMultiSwitchMapNode<
                                     MultiSwitchSrv6TunnelMap,
                                     MultiSwitchSrv6TunnelMapTraits> {
 public:
  using Traits = MultiSwitchSrv6TunnelMapTraits;
  using BaseT = ThriftMultiSwitchMapNode<
      MultiSwitchSrv6TunnelMap,
      MultiSwitchSrv6TunnelMapTraits>;
  using BaseT::modify;

  MultiSwitchSrv6TunnelMap() = default;
  virtual ~MultiSwitchSrv6TunnelMap() = default;

  MultiSwitchSrv6TunnelMap* modify(std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
