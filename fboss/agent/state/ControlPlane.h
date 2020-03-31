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
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_map.hpp>
#include <vector>

namespace facebook::fboss {

class SwitchState;

struct ControlPlaneFields {
  using RxReasonToQueue = std::vector<cfg::PacketRxReasonToQueue>;

  ControlPlaneFields() {}

  template <typename Fn>
  void forEachChild(Fn /*fn*/) {}

  folly::dynamic toFollyDynamic() const;
  static ControlPlaneFields fromFollyDynamic(const folly::dynamic& json);

  QueueConfig queues;
  RxReasonToQueue rxReasonToQueue;
  std::optional<std::string> qosPolicy;
};

/*
 * ControlPlane stores state about path settings of traffic to userver CPU
 * on the switch.
 */
class ControlPlane : public NodeBaseT<ControlPlane, ControlPlaneFields> {
 public:
  using RxReasonToQueue = ControlPlaneFields::RxReasonToQueue;

  ControlPlane() {}

  static std::shared_ptr<ControlPlane> fromFollyDynamic(
      const folly::dynamic& json) {
    const auto& fields = ControlPlaneFields::fromFollyDynamic(json);
    return std::make_shared<ControlPlane>(fields);
  }

  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamic();
  }

  const QueueConfig& getQueues() const {
    return getFields()->queues;
  }
  void resetQueues(QueueConfig& queues) {
    writableFields()->queues.swap(queues);
  }

  const RxReasonToQueue& getRxReasonToQueue() const {
    return getFields()->rxReasonToQueue;
  }
  void resetRxReasonToQueue(RxReasonToQueue& rxReasonToQueue) {
    writableFields()->rxReasonToQueue.swap(rxReasonToQueue);
  }

  const std::optional<std::string>& getQosPolicy() const {
    return getFields()->qosPolicy;
  }
  void resetQosPolicy(std::optional<std::string>& qosPolicy) {
    writableFields()->qosPolicy.swap(qosPolicy);
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
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
