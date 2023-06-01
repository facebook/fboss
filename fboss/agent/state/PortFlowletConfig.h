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

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class PortFlowletCfg;
USE_THRIFT_COW(PortFlowletCfg)
/*
 * PortFlowletCfg stores the port flowlet related properites as need by {port}
 */
class PortFlowletCfg
    : public ThriftStructNode<PortFlowletCfg, state::PortFlowletFields> {
 public:
  using Base = ThriftStructNode<PortFlowletCfg, state::PortFlowletFields>;

  explicit PortFlowletCfg(const std::string& id) {
    set<switch_state_tags::id>(id);
  }
  PortFlowletCfg(
      const std::string& id,
      int scalingFactor,
      int loadWeight,
      int queueWeight) {
    set<switch_state_tags::id>(id);
    setScalingFactor(scalingFactor);
    setLoadWeight(loadWeight);
    setQueueWeight(queueWeight);
  }
  uint16_t getScalingFactor() const {
    return get<switch_state_tags::scalingFactor>()->cref();
  }

  uint16_t getLoadWeight() const {
    return get<switch_state_tags::loadWeight>()->cref();
  }

  uint16_t getQueueWeight() const {
    return get<switch_state_tags::queueWeight>()->cref();
  }

  const std::string& getID() const {
    return cref<switch_state_tags::id>()->cref();
  }

  void setScalingFactor(const uint16_t scalingFactor) {
    set<switch_state_tags::scalingFactor>(scalingFactor);
  }

  void setLoadWeight(const uint16_t loadWeight) {
    set<switch_state_tags::loadWeight>(loadWeight);
  }

  void setQueueWeight(const uint16_t queueWeight) {
    set<switch_state_tags::queueWeight>(queueWeight);
  }

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
