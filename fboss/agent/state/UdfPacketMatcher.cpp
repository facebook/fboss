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

void UdfPacketMatcher::setUdfl2PktType(cfg::UdfMatchL2Type type) {
  set<switch_config_tags::l2PktType>(type);
}

void UdfPacketMatcher::setUdfl3PktType(cfg::UdfMatchL3Type type) {
  set<switch_config_tags::l3pktType>(type);
}

void UdfPacketMatcher::setUdfl4PktType(cfg::UdfMatchL4Type type) {
  set<switch_config_tags::l4PktType>(type);
}

void UdfPacketMatcher::setUdfL4DstPort(std::optional<int> port) {
  if (port) {
    set<switch_config_tags::UdfL4DstPort>(*port);
  } else {
    ref<switch_config_tags::UdfL4DstPort>().reset();
  }
}

template class ThriftStructNode<UdfPacketMatcher, cfg::UdfPacketMatcher>;
} // namespace facebook::fboss
