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
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/types.h"
#include "fboss/agent/state/Thrifty.h"

#include <boost/container/flat_map.hpp>
#include <string>
#include <utility>
#include <vector>
#include <folly/Optional.h>

namespace facebook { namespace fboss {

struct PortQueueFields {
  using AQMMap = boost::container::flat_map<cfg::QueueCongestionBehavior,
                                            cfg::ActiveQueueManagement>;

  template<typename Fn>
  void forEachChild(Fn) {}

  state::PortQueueFields toThrift() const;
  static PortQueueFields fromThrift(state::PortQueueFields const&);

  folly::dynamic toFollyDynamic() const;
  static PortQueueFields fromFollyDynamic(const folly::dynamic& json);
  uint8_t id{0};
  cfg::QueueScheduling scheduling{cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN};
  cfg::StreamType streamType{cfg::StreamType::UNICAST};
  int weight{1};
  folly::Optional<int> reservedBytes{folly::none};
  folly::Optional<cfg::MMUScalingFactor> scalingFactor{folly::none};
  folly::Optional<std::string> name{folly::none};
  folly::Optional<int> packetsPerSec{folly::none};
  folly::Optional<int> sharedBytes{folly::none};
  // Using map to avoid manually sorting aqm list from thrift api
  AQMMap aqms;
};

/*
 * PortQueue defines the behaviour of the per port queues
 */
class PortQueue :
    public ThriftyBaseT<state::PortQueueFields, PortQueue, PortQueueFields> {
 public:
  using AQMMap = PortQueueFields::AQMMap;

  explicit PortQueue(uint8_t id) {
    writableFields()->id = id;
  }

  bool operator==(const PortQueue& queue) const {
    // TODO(joseph5wu) Add sharedBytes
    return getFields()->id == queue.getID() &&
           getFields()->streamType == queue.getStreamType() &&
           getFields()->weight == queue.getWeight() &&
           getFields()->reservedBytes == queue.getReservedBytes() &&
           getFields()->scalingFactor == queue.getScalingFactor() &&
           getFields()->scheduling == queue.getScheduling() &&
           getFields()->aqms == queue.getAqms() &&
           getFields()->packetsPerSec == queue.getPacketsPerSec() &&
           getFields()->name == queue.getName();
  }
  bool operator!=(const PortQueue& queue) const {
    return !(*this == queue);
  }

  std::string toString() const;

  uint8_t getID() const {
    return getFields()->id;
  }

  void setScheduling(cfg::QueueScheduling scheduling) {
    writableFields()->scheduling = scheduling;
    if (scheduling == cfg::QueueScheduling::STRICT_PRIORITY) {
      writableFields()->weight = 0;
    }
  }

  cfg::QueueScheduling getScheduling() const {
    return getFields()->scheduling;
  }

  void setStreamType(cfg::StreamType type) {
    writableFields()->streamType = type;
  }

  cfg::StreamType getStreamType() const {
    return getFields()->streamType;
  }

  int getWeight() const {
    return getFields()->weight;
  }

  void setWeight(int weight) {
    if (getFields()->scheduling != cfg::QueueScheduling::STRICT_PRIORITY) {
      writableFields()->weight = weight;
    }
  }

  folly::Optional<int> getReservedBytes() const {
    return getFields()->reservedBytes;
  }

  void setReservedBytes(int reservedBytes) {
    writableFields()->reservedBytes = reservedBytes;
  }

  folly::Optional<cfg::MMUScalingFactor> getScalingFactor() const {
    return getFields()->scalingFactor;
  }

  void setScalingFactor(cfg::MMUScalingFactor scalingFactor) {
    writableFields()->scalingFactor = scalingFactor;
  }

  const AQMMap& getAqms() const {
    return getFields()->aqms;
  }

  void resetAqms(std::vector<cfg::ActiveQueueManagement> aqms) {
    writableFields()->aqms.clear();
    for (auto& aqm: aqms) {
      writableFields()->aqms.emplace(aqm.behavior, aqm);
    }
  }

  folly::Optional<std::string> getName() const {
    return getFields()->name;
  }
  void setName(const std::string& name) {
    writableFields()->name = name;
  }

  folly::Optional<int> getPacketsPerSec() const {
    return getFields()->packetsPerSec;
  }
  void setPacketsPerSec(int packetsPerSec) {
    writableFields()->packetsPerSec = packetsPerSec;
  }

  folly::Optional<int> getSharedBytes() const {
    return getFields()->sharedBytes;
  }
  void setSharedBytes(int sharedBytes) {
    writableFields()->sharedBytes = sharedBytes;
  }

 private:
  // Inherit the constructors required for clone()
  using ThriftyBaseT<state::PortQueueFields, PortQueue, PortQueueFields>::
      ThriftyBaseT;
  friend class CloneAllocator;
};

using QueueConfig = std::vector<std::shared_ptr<PortQueue>>;

bool comparePortQueueAQMs(
  const PortQueue::AQMMap& aqmMap,
  const std::vector<cfg::ActiveQueueManagement>& aqms);

bool checkSwConfPortQueueMatch(
  const std::shared_ptr<PortQueue>& swQueue,
  const cfg::PortQueue* cfgQueue);
}} // facebook::fboss
