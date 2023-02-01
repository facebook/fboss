/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/DsfNode.h"

#include <folly/MacAddress.h>

#include "fboss/agent/gen-cpp2/switch_config_fatal.h"
#include "fboss/agent/gen-cpp2/switch_config_fatal_types.h"

namespace facebook::fboss {

DsfNode::DsfNode(SwitchID switchId)
    : ThriftStructNode<DsfNode, cfg::DsfNode>() {
  set<switch_config_tags::switchId>(switchId);
}

SwitchID DsfNode::getSwitchId() const {
  return SwitchID(get<switch_config_tags::switchId>()->cref());
}
std::string DsfNode::getName() const {
  return get<switch_config_tags::name>()->cref();
}
void DsfNode::setName(const std::string& name) {
  set<switch_config_tags::name>(name);
}

cfg::DsfNodeType DsfNode::getType() const {
  return get<switch_config_tags::type>()->cref();
}

cfg::AsicType DsfNode::getAsicType() const {
  return get<switch_config_tags::asicType>()->cref();
}

void DsfNode::setLoopbackIps(const std::vector<std::string>& loopbackIps) {
  set<switch_config_tags::loopbackIps>(loopbackIps);
}

cfg::Range64 DsfNode::getSystemPortRange() const {
  return get<switch_config_tags::systemPortRange>()->toThrift();
}

folly::MacAddress DsfNode::getMac() const {
  return folly::MacAddress(get<switch_config_tags::nodeMac>()->cref());
}

std::shared_ptr<DsfNode> DsfNode::fromFollyDynamic(
    const folly::dynamic& entry) {
  auto node = std::make_shared<DsfNode>();
  static_cast<std::shared_ptr<BaseT>>(node)->fromFollyDynamic(entry);
  return node;
}

std::set<folly::CIDRNetwork> DsfNode::getLoopbackIpsSorted() const {
  std::set<folly::CIDRNetwork> subnets;
  const auto& loopbackIps = *getLoopbackIps();
  for (auto loopbackSubnet : loopbackIps) {
    subnets.emplace(folly::IPAddress::createNetwork(
        loopbackSubnet->toThrift(), -1, false /*don't apply mask*/));
  }
  return subnets;
}

template class ThriftStructNode<DsfNode, cfg::DsfNode>;
} // namespace facebook::fboss
