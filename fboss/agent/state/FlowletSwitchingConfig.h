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

class FlowletSwitchingConfig;
USE_THRIFT_COW(FlowletSwitchingConfig);

class FlowletSwitchingConfig : public ThriftStructNode<
                                   FlowletSwitchingConfig,
                                   cfg::FlowletSwitchingConfig> {
 public:
  using BaseT =
      ThriftStructNode<FlowletSwitchingConfig, cfg::FlowletSwitchingConfig>;
  explicit FlowletSwitchingConfig() {}

  void setInactivityIntervalUsecs(const uint16_t inactivityIntervalUsecs) {
    set<switch_config_tags::inactivityIntervalUsecs>(inactivityIntervalUsecs);
  }

  uint16_t getInactivityIntervalUsecs() const {
    return get<switch_config_tags::inactivityIntervalUsecs>()->cref();
  }

  void setFlowletTableSize(const uint16_t flowletTableSize) {
    set<switch_config_tags::flowletTableSize>(flowletTableSize);
  }

  uint16_t getFlowletTableSize() const {
    return get<switch_config_tags::flowletTableSize>()->cref();
  }

  void setDynamicEgressLoadExponent(const uint16_t dynamicEgressLoadExponent) {
    set<switch_config_tags::dynamicEgressLoadExponent>(
        dynamicEgressLoadExponent);
  }

  uint16_t getDynamicEgressLoadExponent() const {
    return get<switch_config_tags::dynamicEgressLoadExponent>()->cref();
  }

  void setDynamicQueueExponent(const uint16_t dynamicQueueExponent) {
    set<switch_config_tags::dynamicQueueExponent>(dynamicQueueExponent);
  }

  uint16_t getDynamicQueueExponent() const {
    return get<switch_config_tags::dynamicQueueExponent>()->cref();
  }

  void setDynamicQueueMinThresholdBytes(
      const uint32_t dynamicQueueMinThresholdBytes) {
    set<switch_config_tags::dynamicQueueMinThresholdBytes>(
        dynamicQueueMinThresholdBytes);
  }

  uint16_t getDynamicQueueMinThresholdBytes() const {
    return get<switch_config_tags::dynamicQueueMinThresholdBytes>()->cref();
  }

  void setDynamicQueueMaxThresholdBytes(
      const uint32_t dynamicQueueMaxThresholdBytes) {
    set<switch_config_tags::dynamicQueueMaxThresholdBytes>(
        dynamicQueueMaxThresholdBytes);
  }

  uint16_t getDynamicQueueMaxThresholdBytes() const {
    return get<switch_config_tags::dynamicQueueMaxThresholdBytes>()->cref();
  }

  void setDynamicSampleRate(const uint32_t dynamicSampleRate) {
    set<switch_config_tags::dynamicSampleRate>(dynamicSampleRate);
  }

  uint16_t getDynamicSampleRate() const {
    return get<switch_config_tags::dynamicSampleRate>()->cref();
  }

  void setPortScalingFactor(std::optional<uint16_t> portScalingFactor) {
    if (!portScalingFactor) {
      ref<switch_config_tags::portScalingFactor>().reset();
    } else {
      set<switch_config_tags::portScalingFactor>(*portScalingFactor);
    }
  }

  std::optional<uint16_t> getPortScalingFactor() const {
    if (auto portScalingFactor = get<switch_config_tags::portScalingFactor>()) {
      return portScalingFactor->cref();
    }
    return std::nullopt;
  }

  void setPortLoadWeight(std::optional<uint16_t> portLoadWeight) {
    if (!portLoadWeight) {
      ref<switch_config_tags::portLoadWeight>().reset();
    } else {
      set<switch_config_tags::portLoadWeight>(*portLoadWeight);
    }
  }

  std::optional<uint16_t> getPortLoadWeight() const {
    if (auto portLoadWeight = get<switch_config_tags::portLoadWeight>()) {
      return portLoadWeight->cref();
    }
    return std::nullopt;
  }

  void setPortQueueWeight(std::optional<uint16_t> portQueueWeight) {
    if (!portQueueWeight) {
      ref<switch_config_tags::portQueueWeight>().reset();
    } else {
      set<switch_config_tags::portQueueWeight>(*portQueueWeight);
    }
  }

  std::optional<uint16_t> getPortQueueWeight() const {
    if (auto portQueueWeight = get<switch_config_tags::portQueueWeight>()) {
      return portQueueWeight->cref();
    }
    return std::nullopt;
  }

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
