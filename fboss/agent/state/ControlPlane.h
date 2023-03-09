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
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_map.hpp>
#include <optional>
#include <vector>

namespace facebook::fboss {

class SwitchState;

USE_THRIFT_COW(ControlPlane);

/*
 * ControlPlane stores state about path settings of traffic to userver CPU
 * on the switch.
 */
class ControlPlane
    : public ThriftStructNode<ControlPlane, state::ControlPlaneFields> {
 public:
  using BaseT = ThriftStructNode<ControlPlane, state::ControlPlaneFields>;
  using BaseT::modify;
  using PortQueues =
      BaseT::Fields::NamedMemberTypes::type_of<switch_state_tags::queues>;
  using PacketRxReasonToQueue = BaseT::Fields::NamedMemberTypes::type_of<
      switch_state_tags::rxReasonToQueue>;

  using RxReasonToQueue = std::vector<cfg::PacketRxReasonToQueue>;

  ControlPlane() {}

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
