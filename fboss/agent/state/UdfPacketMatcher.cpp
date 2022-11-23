/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/UdfPacketMatcher.h"
#include "fboss/agent/gen-cpp2/switch_config_fatal.h"
#include "fboss/agent/gen-cpp2/switch_config_fatal_types.h"

namespace facebook::fboss {

UdfPacketMatcher::UdfPacketMatcher(const std::string& name)
    : ThriftStructNode<UdfPacketMatcher, cfg::UdfPacketMatcher>() {
  set<switch_config_tags::name>(name);
}

std::string UdfPacketMatcher::getName() const {
  return get<switch_config_tags::name>()->cref();
}

std::shared_ptr<UdfPacketMatcher> UdfPacketMatcher::fromFollyDynamic(
    const folly::dynamic& entry) {
  auto node = std::make_shared<UdfPacketMatcher>();
  static_cast<std::shared_ptr<BaseT>>(node)->fromFollyDynamic(entry);
  return node;
}

cfg::UdfMatchL2Type UdfPacketMatcher::getUdfl2PktType() const {
  return get<switch_config_tags::l2PktType>()->cref();
}

cfg::UdfMatchL3Type UdfPacketMatcher::getUdfl3PktType() const {
  return get<switch_config_tags::l3pktType>()->cref();
}

cfg::UdfMatchL4Type UdfPacketMatcher::getUdfl4PktType() const {
  return get<switch_config_tags::l4PktType>()->cref();
}

std::optional<int> UdfPacketMatcher::getUdfL4DstPort() const {
  auto l4DstPort = get<switch_config_tags::UdfL4DstPort>();
  if (l4DstPort) {
    return l4DstPort->cref();
  }
  return std::nullopt;
}

template class ThriftStructNode<UdfPacketMatcher, cfg::UdfPacketMatcher>;
} // namespace facebook::fboss
