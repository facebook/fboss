/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

USE_THRIFT_COW(DsfNode);

class DsfNode : public ThriftStructNode<DsfNode, cfg::DsfNode> {
 public:
  using BaseT = ThriftStructNode<DsfNode, cfg::DsfNode>;
  explicit DsfNode(SwitchID switchId);

  SwitchID getID() const {
    return getSwitchId();
  }
  SwitchID getSwitchId() const;
  std::string getName() const;
  void setName(const std::string& name);
  cfg::DsfNodeType getType() const;
  void setType(cfg::DsfNodeType type);
  cfg::AsicType getAsicType() const;

  auto getLoopbackIps() const {
    return safe_cref<switch_config_tags::loopbackIps>();
  }
  std::set<folly::CIDRNetwork> getLoopbackIpsSorted() const;
  void setLoopbackIps(const std::vector<std::string>& loopbackIps);
  cfg::SystemPortRanges getSystemPortRanges() const;
  std::optional<folly::MacAddress> getMac() const;
  PlatformType getPlatformType() const;
  std::optional<int> getClusterId() const;
  std::optional<int> getLocalSystemPortOffset() const;
  std::optional<int> getGlobalSystemPortOffset() const;
  std::optional<int> getInbandPortId() const;
  cfg::QueueScheduling getScheduling() const;
  std::optional<cfg::SchedulingParam> getSchedulingParam() const;
  void setLocalSystemPortOffset(std::optional<int> val);
  void setGlobalSystemPortOffset(std::optional<int> val);
  std::optional<int> getFabricLevel() const;
  bool isLevel2FabricNode() const;
  bool isInterfaceNode() const;

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
