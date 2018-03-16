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

#include <string>
#include <utility>
#include <folly/Optional.h>

namespace facebook { namespace fboss {

struct PortQueueFields {
  explicit PortQueueFields(uint8_t id) : id(id) {}

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
  folly::Optional<cfg::ActiveQueueManagement> aqm{folly::none};
};

/*
 * PortQueue defines the behaviour of the per port queues
 */
class PortQueue :
    public ThriftyBaseT<state::PortQueueFields, PortQueue, PortQueueFields> {
 public:
  explicit PortQueue(uint8_t id);
  bool operator==(const PortQueue& queue) const {
    return getFields()->id == queue.getID() &&
           getFields()->streamType == queue.getStreamType() &&
           getFields()->weight == queue.getWeight() &&
           getFields()->reservedBytes == queue.getReservedBytes() &&
           getFields()->scalingFactor == queue.getScalingFactor() &&
           getFields()->scheduling == queue.getScheduling() &&
           getFields()->aqm == queue.getAqm();
  }
  bool operator!=(const PortQueue& queue) {
    return !(*this == queue);
  }

  uint8_t getID() const {
    return getFields()->id;
  }

  void setScheduling(cfg::QueueScheduling scheduling) {
    writableFields()->scheduling = scheduling;
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
    writableFields()->weight = weight;
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

  folly::Optional<cfg::ActiveQueueManagement> getAqm() const {
    return getFields()->aqm;
  }

  void setAqm(const cfg::ActiveQueueManagement& aqm) {
    writableFields()->aqm = aqm;
  }

 private:
  // Inherit the constructors required for clone()
  using ThriftyBaseT<state::PortQueueFields, PortQueue, PortQueueFields>::
      ThriftyBaseT;
  friend class CloneAllocator;
};

}} // facebook::fboss
