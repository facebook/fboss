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

  uint32_t getDynamicQueueMinThresholdBytes() const {
    return get<switch_config_tags::dynamicQueueMinThresholdBytes>()->cref();
  }

  void setDynamicQueueMaxThresholdBytes(
      const uint32_t dynamicQueueMaxThresholdBytes) {
    set<switch_config_tags::dynamicQueueMaxThresholdBytes>(
        dynamicQueueMaxThresholdBytes);
  }

  uint32_t getDynamicQueueMaxThresholdBytes() const {
    return get<switch_config_tags::dynamicQueueMaxThresholdBytes>()->cref();
  }

  void setDynamicSampleRate(const uint32_t dynamicSampleRate) {
    set<switch_config_tags::dynamicSampleRate>(dynamicSampleRate);
  }

  uint32_t getDynamicSampleRate() const {
    return get<switch_config_tags::dynamicSampleRate>()->cref();
  }

  void setDynamicEgressMinThresholdBytes(
      const uint32_t dynamicEgressMinThresholdBytes) {
    set<switch_config_tags::dynamicEgressMinThresholdBytes>(
        dynamicEgressMinThresholdBytes);
  }

  uint32_t getDynamicEgressMinThresholdBytes() const {
    return get<switch_config_tags::dynamicEgressMinThresholdBytes>()->cref();
  }

  void setDynamicEgressMaxThresholdBytes(
      const uint32_t dynamicEgressMaxThresholdBytes) {
    set<switch_config_tags::dynamicEgressMaxThresholdBytes>(
        dynamicEgressMaxThresholdBytes);
  }

  uint32_t getDynamicEgressMaxThresholdBytes() const {
    return get<switch_config_tags::dynamicEgressMaxThresholdBytes>()->cref();
  }

  void setDynamicPhysicalQueueExponent(
      const uint16_t dynamicPhysicalQueueExponent) {
    set<switch_config_tags::dynamicPhysicalQueueExponent>(
        dynamicPhysicalQueueExponent);
  }

  uint16_t getDynamicPhysicalQueueExponent() const {
    return get<switch_config_tags::dynamicPhysicalQueueExponent>()->cref();
  }

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
