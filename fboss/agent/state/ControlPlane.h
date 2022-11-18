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

#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_map.hpp>
#include <optional>
#include <vector>

namespace facebook::fboss {

class SwitchState;

struct ControlPlaneFields
    : public ThriftyFields<ControlPlaneFields, state::ControlPlaneFields> {
  using RxReasonToQueue = std::vector<cfg::PacketRxReasonToQueue>;

  ControlPlaneFields() : ControlPlaneFields(state::ControlPlaneFields{}) {}

  explicit ControlPlaneFields(const state::ControlPlaneFields& fields) {
    writableData() = fields;
  }

  template <typename Fn>
  void forEachChild(Fn /*fn*/) {}

  folly::dynamic toFollyDynamicLegacy() const;
  static ControlPlaneFields fromFollyDynamicLegacy(const folly::dynamic& json);

  state::ControlPlaneFields toThrift() const override;
  static ControlPlaneFields fromThrift(state::ControlPlaneFields const& fields);
  static folly::dynamic migrateToThrifty(folly::dynamic const& dyn);
  static void migrateFromThrifty(folly::dynamic& dyn);

  bool operator==(const ControlPlaneFields& other) const {
    return data() == other.data();
  }

  QueueConfig queues() const {
    QueueConfig queues;
    for (auto queue : *data().queues()) {
      queues.push_back(std::make_shared<PortQueue>(queue));
    }
    return queues;
  }

  void resetQueues() {
    writableData().queues()->clear();
  }

  void setQueues(QueueConfig queues) {
    resetQueues();
    for (auto queue : queues) {
      writableData().queues()->push_back(queue->toThrift());
    }
  }

  RxReasonToQueue rxReasonToQueue() const {
    RxReasonToQueue rxReasonToQueue;
    for (auto entry : *data().rxReasonToQueue()) {
      rxReasonToQueue.push_back(entry);
    }
    return rxReasonToQueue;
  }

  void resetRxReasonToQueue() {
    writableData().rxReasonToQueue()->clear();
  }
  void setRxReasonToQueue(RxReasonToQueue rxReasonToQueue) {
    resetRxReasonToQueue();
    for (auto entry : rxReasonToQueue) {
      writableData().rxReasonToQueue()->push_back(entry);
    }
  }

  std::optional<std::string> qosPolicy() const {
    if (auto policy = data().defaultQosPolicy()) {
      return *policy;
    }
    return std::nullopt;
  }

  void setQosPolicy(std::optional<std::string> policy) {
    if (!policy) {
      writableData().defaultQosPolicy().reset();
      return;
    }
    writableData().defaultQosPolicy() = *policy;
  }
};

/*
 * ControlPlane stores state about path settings of traffic to userver CPU
 * on the switch.
 */
class ControlPlane : public ThriftyBaseT<
                         state::ControlPlaneFields,
                         ControlPlane,
                         ControlPlaneFields> {
 public:
  using RxReasonToQueue = ControlPlaneFields::RxReasonToQueue;

  ControlPlane() {}

  static std::shared_ptr<ControlPlane> fromFollyDynamicLegacy(
      const folly::dynamic& json) {
    const auto& fields = ControlPlaneFields::fromFollyDynamicLegacy(json);
    return std::make_shared<ControlPlane>(fields);
  }

  folly::dynamic toFollyDynamicLegacy() const {
    return getFields()->toFollyDynamicLegacy();
  }

  QueueConfig getQueues() const {
    return getFields()->queues();
  }
  void resetQueues(QueueConfig& queues) {
    writableFields()->setQueues(queues);
  }

  RxReasonToQueue getRxReasonToQueue() const {
    return getFields()->rxReasonToQueue();
  }
  void resetRxReasonToQueue(RxReasonToQueue& rxReasonToQueue) {
    writableFields()->setRxReasonToQueue(rxReasonToQueue);
  }

  std::optional<std::string> getQosPolicy() const {
    return getFields()->qosPolicy();
  }
  void resetQosPolicy(std::optional<std::string>& qosPolicy) {
    writableFields()->setQosPolicy(qosPolicy);
  }

  ControlPlane* modify(std::shared_ptr<SwitchState>* state);

  bool operator==(const ControlPlane& controlPlane) const;
  bool operator!=(const ControlPlane& controlPlane) {
    return !(*this == controlPlane);
  }

  static cfg::PacketRxReasonToQueue makeRxReasonToQueueEntry(
      cfg::PacketRxReason reason,
      uint16_t queueId);

 private:
  // Inherit the constructors required for clone()
  using ThriftyBaseT::ThriftyBaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
