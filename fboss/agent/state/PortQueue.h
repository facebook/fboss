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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/BufferPoolConfig.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_map.hpp>
#include <sys/types.h>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace facebook::fboss {

using BufferPoolCfgPtr = std::shared_ptr<BufferPoolCfg>;

USE_THRIFT_COW(PortQueue)
RESOLVE_STRUCT_MEMBER(PortQueue, ctrl_if_tags::bufferPoolConfig, BufferPoolCfg);

// TODO: add resolver for thrift list and a mechanism for thrift struct node to
// pick that resolver for member list.

template <>
struct thrift_cow::ThriftStructResolver<PortQueueFields> {
  using type = PortQueue;
};

/*
 * PortQueue defines the behaviour of the per port queues
 */
class PortQueue : public thrift_cow::ThriftStructNode<PortQueueFields> {
 public:
  using Base = thrift_cow::ThriftStructNode<PortQueueFields>;
  using AQMMap = boost::container::
      flat_map<cfg::QueueCongestionBehavior, cfg::ActiveQueueManagement>;
  using AqmsType = typename Base::Fields::TypeFor<ctrl_if_tags::aqms>;
  using PortQueueRateType =
      typename Base::Fields::TypeFor<ctrl_if_tags::portQueueRate>;

  explicit PortQueue(uint8_t id) {
    set<ctrl_if_tags::id>(id);
  }

  std::string toString() const;

  uint8_t getID() const {
    return cref<ctrl_if_tags::id>()->cref();
  }

  void setScheduling(cfg::QueueScheduling scheduling) {
    set<ctrl_if_tags::scheduling>(apache::thrift::util::enumName(scheduling));
    switch (scheduling) {
      case cfg::QueueScheduling::STRICT_PRIORITY:
      case cfg::QueueScheduling::INTERNAL:
        set<ctrl_if_tags::weight>(0);
        break;
      case cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN:
      case cfg::QueueScheduling::DEFICIT_ROUND_ROBIN:
        break;
    }
  }

  cfg::QueueScheduling getScheduling() const {
    const auto& name = cref<ctrl_if_tags::scheduling>()->cref();
    return apache::thrift::util::enumValueOrThrow<cfg::QueueScheduling>(name);
  }

  void setStreamType(cfg::StreamType type) {
    set<ctrl_if_tags::streamType>(apache::thrift::util::enumName(type));
  }

  cfg::StreamType getStreamType() const {
    const auto& name = cref<ctrl_if_tags::streamType>()->cref();
    return apache::thrift::util::enumValueOrThrow<cfg::StreamType>(name);
  }

  int getWeight() const {
    return cref<ctrl_if_tags::weight>()->cref();
  }

  void setWeight(int weight) {
    if (getScheduling() != cfg::QueueScheduling::STRICT_PRIORITY) {
      set<ctrl_if_tags::weight>(weight);
    }
  }

  std::optional<int> getReservedBytes() const {
    if (const auto& reserved = cref<ctrl_if_tags::reserved>()) {
      return std::optional<int>(reserved->cref());
    }
    return std::nullopt;
  }

  void setReservedBytes(int reservedBytes) {
    set<ctrl_if_tags::reserved>(reservedBytes);
  }

  std::optional<int> getMaxDynamicSharedBytes() const {
    if (const auto& maxDynamicSharedBytes =
            cref<ctrl_if_tags::maxDynamicSharedBytes>()) {
      return std::optional<int>(maxDynamicSharedBytes->cref());
    }
    return std::nullopt;
  }

  void setMaxDynamicSharedBytes(int maxDynamicSharedBytes) {
    set<ctrl_if_tags::maxDynamicSharedBytes>(maxDynamicSharedBytes);
  }

  std::optional<cfg::MMUScalingFactor> getScalingFactor() const {
    if (const auto& scalingFactor = cref<switch_state_tags::scalingFactor>()) {
      return apache::thrift::util::enumValueOrThrow<cfg::MMUScalingFactor>(
          scalingFactor->cref());
    }
    return std::nullopt;
  }

  void setScalingFactor(cfg::MMUScalingFactor scalingFactor) {
    set<switch_state_tags::scalingFactor>(
        apache::thrift::util::enumName(scalingFactor));
  }

  const auto& getAqms() const {
    return cref<ctrl_if_tags::aqms>();
  }

  void resetAqms(std::vector<cfg::ActiveQueueManagement> aqms) {
    if (!aqms.empty()) {
      set<ctrl_if_tags::aqms>(std::move(aqms));
    } else {
      ref<ctrl_if_tags::aqms>().reset();
    }
  }

  std::optional<std::string> getName() const {
    if (const auto& name = cref<ctrl_if_tags::name>()) {
      return name->cref();
    }
    return std::nullopt;
  }
  void setName(const std::string& name) {
    set<ctrl_if_tags::name>(name);
  }
  std::optional<int> getSharedBytes() const {
    if (const auto& sharedBytes = cref<common_if_tags::sharedBytes>()) {
      return sharedBytes->cref();
    }
    return std::nullopt;
  }
  void setSharedBytes(int sharedBytes) {
    set<common_if_tags::sharedBytes>(sharedBytes);
  }

  const auto& getPortQueueRate() const {
    return cref<ctrl_if_tags::portQueueRate>();
  }

  void setPortQueueRate(cfg::PortQueueRate portQueueRate) {
    set<ctrl_if_tags::portQueueRate>(std::move(portQueueRate));
  }

  std::optional<int> getBandwidthBurstMinKbits() const {
    if (const auto& bandwidthBurstMinKbits =
            cref<ctrl_if_tags::bandwidthBurstMinKbits>()) {
      return bandwidthBurstMinKbits->cref();
    }
    return std::nullopt;
  }

  void setBandwidthBurstMinKbits(int bandwidthBurstMinKbits) {
    set<ctrl_if_tags::bandwidthBurstMinKbits>(bandwidthBurstMinKbits);
  }

  std::optional<int> getBandwidthBurstMaxKbits() const {
    if (const auto& bandwidthBurstMaxKbits =
            cref<ctrl_if_tags::bandwidthBurstMaxKbits>()) {
      return bandwidthBurstMaxKbits->cref();
    }
    return std::nullopt;
  }

  void setBandwidthBurstMaxKbits(int bandwidthBurstMaxKbits) {
    set<ctrl_if_tags::bandwidthBurstMinKbits>(bandwidthBurstMaxKbits);
  }

  std::optional<TrafficClass> getTrafficClass() const {
    if (const auto& trafficClass = cref<switch_state_tags::trafficClass>()) {
      return static_cast<TrafficClass>(trafficClass->cref());
    }
    return std::nullopt;
  }

  void setTrafficClasses(TrafficClass trafficClass) {
    set<switch_state_tags::trafficClass>(trafficClass);
  }

  // THRIFT_COPY: change from list to set in thrift file and use thrift set node
  // directly
  std::optional<std::set<PfcPriority>> getPfcPrioritySet() const {
    if (const auto& pfcPriorities = cref<switch_state_tags::pfcPriorities>()) {
      std::set<PfcPriority> returnValue{};
      for (const auto& pfcPriority : std::as_const(*pfcPriorities)) {
        returnValue.insert(static_cast<PfcPriority>(pfcPriority->cref()));
      }
      return returnValue;
    }
    return std::nullopt;
  }

  void setPfcPrioritySet(std::set<PfcPriority> pfcPrioritySet) {
    if (!pfcPrioritySet.empty()) {
      std::vector<int16_t> vec{
          std::begin(pfcPrioritySet), std::end(pfcPrioritySet)};
      set<switch_state_tags::pfcPriorities>(vec);
    } else {
      ref<switch_state_tags::pfcPriorities>().reset();
    }
  }

  std::optional<std::string> getBufferPoolName() const {
    if (const auto& bufferPoolName = cref<ctrl_if_tags::bufferPoolName>()) {
      return std::optional<std::string>(bufferPoolName->cref());
    }
    return std::nullopt;
  }

  void setBufferPoolName(const std::string& bufferPoolName) {
    set<ctrl_if_tags::bufferPoolName>(bufferPoolName);
  }

  std::optional<BufferPoolCfgPtr> getBufferPoolConfig() const {
    if (auto bufferPoolConfigPtr =
            safe_cref<ctrl_if_tags::bufferPoolConfig>()) {
      return bufferPoolConfigPtr;
    }
    return std::nullopt;
  }

  void setBufferPoolConfig(BufferPoolCfgPtr bufferPoolConfigPtr) {
    ref<ctrl_if_tags::bufferPoolConfig>() = bufferPoolConfigPtr;
  }

  bool isAqmsSame(const PortQueue* other) const;

  bool isPortQueueRateSame(const PortQueue* other) const {
    if (!other) {
      return false;
    }
    const auto thisRate = get<ctrl_if_tags::portQueueRate>();
    const auto thatRate = other->get<ctrl_if_tags::portQueueRate>();
    if (thisRate == nullptr && thatRate == nullptr) {
      return true;
    } else if (thisRate == nullptr) {
      return false;
    } else if (thatRate == nullptr) {
      return false;
    }
    return thisRate->toThrift() == thatRate->toThrift();
  }

  std::optional<cfg::QueueCongestionDetection> findDetectionInAqms(
      cfg::QueueCongestionBehavior behavior) const;

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

using QueueConfig = std::vector<std::shared_ptr<PortQueue>>;

bool checkSwConfPortQueueMatch(
    const std::shared_ptr<PortQueue>& swQueue,
    const cfg::PortQueue* cfgQueue);

} // namespace facebook::fboss
