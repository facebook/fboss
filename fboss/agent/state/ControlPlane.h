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

USE_THRIFT_COW(ControlPlane);

/*
 * ControlPlane stores state about path settings of traffic to userver CPU
 * on the switch.
 */
class ControlPlane
    : public ThriftStructNode<ControlPlane, state::ControlPlaneFields> {
 public:
  using BaseT = ThriftStructNode<ControlPlane, state::ControlPlaneFields>;
  using PortQueues =
      BaseT::Fields::NamedMemberTypes::type_of<switch_state_tags::queues>;
  using PacketRxReasonToQueue = BaseT::Fields::NamedMemberTypes::type_of<
      switch_state_tags::rxReasonToQueue>;

  using RxReasonToQueue = ControlPlaneFields::RxReasonToQueue;

  ControlPlane() {}

  static std::shared_ptr<ControlPlane> fromFollyDynamicLegacy(
      const folly::dynamic& json) {
    const auto& fields = ControlPlaneFields::fromFollyDynamicLegacy(json);
    return std::make_shared<ControlPlane>(fields.toThrift());
  }

  static std::shared_ptr<ControlPlane> fromFollyDynamic(
      const folly::dynamic& json) {
    const auto& fields = ControlPlaneFields::fromFollyDynamic(json);
    return std::make_shared<ControlPlane>(fields.toThrift());
  }

  folly::dynamic toFollyDynamicLegacy() const {
    auto fields = ControlPlaneFields::fromThrift(toThrift());
    return fields.toFollyDynamicLegacy();
  }

  folly::dynamic toFollyDynamic() const override {
    auto fields = ControlPlaneFields::fromThrift(toThrift());
    return fields.toFollyDynamic();
  }

  const auto& getQueues() const {
    return cref<switch_state_tags::queues>();
  }
  void resetQueues(QueueConfig& queues) {
    std::vector<state::PortQueueFields> queuesThrift{};
    for (auto queue : queues) {
      queuesThrift.push_back(queue->toThrift());
    }
    set<switch_state_tags::queues>(std::move(queuesThrift));
  }

  const auto& getRxReasonToQueue() const {
    return cref<switch_state_tags::rxReasonToQueue>();
  }
  void resetRxReasonToQueue(RxReasonToQueue& rxReasonToQueue) {
    set<switch_state_tags::rxReasonToQueue>(std::move(rxReasonToQueue));
  }

  const auto& getQosPolicy() const {
    return cref<switch_state_tags::defaultQosPolicy>();
  }
  void resetQosPolicy(std::optional<std::string>& qosPolicy) {
    if (qosPolicy) {
      set<switch_state_tags::defaultQosPolicy>(*qosPolicy);
    } else {
      ref<switch_state_tags::defaultQosPolicy>().reset();
    }
  }

  // THRIFT_COPY
  const QueueConfig& getQueuesConfig() const {
    const auto& queues = getQueues();
    return queues->impl();
  }

  ControlPlane* modify(std::shared_ptr<SwitchState>* state);

  static cfg::PacketRxReasonToQueue makeRxReasonToQueueEntry(
      cfg::PacketRxReason reason,
      uint16_t queueId);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
