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

class UdfPacketMatcher;
USE_THRIFT_COW(UdfPacketMatcher);

class UdfPacketMatcher
    : public ThriftStructNode<UdfPacketMatcher, cfg::UdfPacketMatcher> {
 public:
  using BaseT = ThriftStructNode<UdfPacketMatcher, cfg::UdfPacketMatcher>;
  explicit UdfPacketMatcher(const std::string& name);

  std::string getID() const {
    return getName();
  }

  std::string getName() const;
  cfg::UdfMatchL2Type getUdfl2PktType() const;
  cfg::UdfMatchL3Type getUdfl3PktType() const;
  cfg::UdfMatchL4Type getUdfl4PktType() const;
  std::optional<int> getUdfL4DstPort() const;
  void setUdfl2PktType(cfg::UdfMatchL2Type type);
  void setUdfl3PktType(cfg::UdfMatchL3Type type);
  void setUdfl4PktType(cfg::UdfMatchL4Type type);
  void setUdfL4DstPort(std::optional<int> port);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
