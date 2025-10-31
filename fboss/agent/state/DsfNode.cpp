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

void DsfNode::setLocalSystemPortOffset(std::optional<int> val) {
  if (val.has_value()) {
    set<switch_config_tags::localSystemPortOffset>(val.value());
  } else {
    remove<switch_config_tags::localSystemPortOffset>();
  }
}

void DsfNode::setGlobalSystemPortOffset(std::optional<int> val) {
  if (val.has_value()) {
    set<switch_config_tags::globalSystemPortOffset>(val.value());
  } else {
    remove<switch_config_tags::globalSystemPortOffset>();
  }
}

cfg::DsfNodeType DsfNode::getType() const {
  return get<switch_config_tags::type>()->cref();
}

void DsfNode::setType(cfg::DsfNodeType type) {
  set<switch_config_tags::type>(type);
}

cfg::AsicType DsfNode::getAsicType() const {
  return get<switch_config_tags::asicType>()->cref();
}

PlatformType DsfNode::getPlatformType() const {
  return get<switch_config_tags::platformType>()->cref();
}

void DsfNode::setLoopbackIps(const std::vector<std::string>& loopbackIps) {
  set<switch_config_tags::loopbackIps>(loopbackIps);
}

cfg::SystemPortRanges DsfNode::getSystemPortRanges() const {
  auto ranges = get<switch_config_tags::systemPortRanges>()->toThrift();
  return ranges;
}

std::optional<folly::MacAddress> DsfNode::getMac() const {
  std::optional<folly::MacAddress> mac;
  if (get<switch_config_tags::nodeMac>().has_value()) {
    mac = folly::MacAddress(get<switch_config_tags::nodeMac>()->cref());
  }
  return mac;
}

std::set<folly::CIDRNetwork> DsfNode::getLoopbackIpsSorted() const {
  std::set<folly::CIDRNetwork> subnets;
  const auto& loopbackIps = *getLoopbackIps();
  for (auto loopbackSubnet : loopbackIps) {
    subnets.emplace(
        folly::IPAddress::createNetwork(
            loopbackSubnet->toThrift(), -1, false /*don't apply mask*/));
  }
  return subnets;
}

std::optional<int> DsfNode::getClusterId() const {
  std::optional<int> clusterId;
  if (get<switch_config_tags::clusterId>().has_value()) {
    clusterId = get<switch_config_tags::clusterId>()->cref();
  }
  return clusterId;
}

std::optional<int> DsfNode::getInbandPortId() const {
  std::optional<int> inbandPortId;
  if (get<switch_config_tags::inbandPortId>().has_value()) {
    inbandPortId = get<switch_config_tags::inbandPortId>()->cref();
  }
  return inbandPortId;
}

std::optional<int> DsfNode::getFabricLevel() const {
  std::optional<int> fabricLevel;
  if (get<switch_config_tags::fabricLevel>().has_value()) {
    fabricLevel = get<switch_config_tags::fabricLevel>()->cref();
  }
  return fabricLevel;
}

bool DsfNode::isLevel2FabricNode() const {
  auto fabricLevel = getFabricLevel();
  return fabricLevel.has_value() && fabricLevel.value() == 2;
}

bool DsfNode::isInterfaceNode() const {
  return getType() == cfg::DsfNodeType::INTERFACE_NODE;
}

std::optional<int> DsfNode::getLocalSystemPortOffset() const {
  std::optional<int> ret;
  if (get<switch_config_tags::localSystemPortOffset>().has_value()) {
    ret = get<switch_config_tags::localSystemPortOffset>()->cref();
  }
  return ret;
}

std::optional<int> DsfNode::getGlobalSystemPortOffset() const {
  std::optional<int> ret;
  if (get<switch_config_tags::globalSystemPortOffset>().has_value()) {
    ret = get<switch_config_tags::globalSystemPortOffset>()->cref();
  }
  return ret;
}

cfg::QueueScheduling DsfNode::getScheduling() const {
  return get<switch_config_tags::scheduling>()->cref();
}

std::optional<cfg::SchedulingParam> DsfNode::getSchedulingParam() const {
  if (const auto& param = cref<switch_config_tags::schedulingParam>()) {
    return param->toThrift();
  }
  return std::nullopt;
}

template struct ThriftStructNode<DsfNode, cfg::DsfNode>;
} // namespace facebook::fboss
