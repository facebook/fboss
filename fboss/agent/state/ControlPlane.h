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
#include "fboss/agent/types.h"
#include "fboss/agent/state/NodeBase.h"

#include <boost/container/flat_map.hpp>
#include <vector>

namespace facebook { namespace fboss {

class PortQueue;
class SwitchState;

struct ControlPlaneFields {
  using CPUQueueConfig = std::vector<std::shared_ptr<PortQueue>>;
  using RxReasonToQueue =
    boost::container::flat_map<cfg::PacketRxReason, uint8_t>;

  ControlPlaneFields() {}

  template <typename Fn>
  void forEachChild(Fn /*fn*/) {}

  folly::dynamic toFollyDynamic() const;
  static ControlPlaneFields fromFollyDynamic(const folly::dynamic& json);

  CPUQueueConfig queues;
  RxReasonToQueue rxReasonToQueue;
};

/*
 * ControlPlane stores state about path settings of traffic to userver CPU
 * on the switch.
 */
class ControlPlane : public NodeBaseT<ControlPlane, ControlPlaneFields> {
public:
  using CPUQueueConfig = ControlPlaneFields::CPUQueueConfig;
  using RxReasonToQueue = ControlPlaneFields::RxReasonToQueue;

  ControlPlane() {}

  static std::shared_ptr<ControlPlane>
  fromFollyDynamic(const folly::dynamic& json) {
    const auto& fields = ControlPlaneFields::fromFollyDynamic(json);
    return std::make_shared<ControlPlane>(fields);
  }

  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamic();
  }

  const CPUQueueConfig& getQueues() const {
    return getFields()->queues;
  }
  void resetQueues(CPUQueueConfig& queues) {
    writableFields()->queues.swap(queues);
  }

  const RxReasonToQueue& getRxReasonToQueue() const {
    return getFields()->rxReasonToQueue;
  }
  void resetRxReasonToQueue(RxReasonToQueue& rxReasonToQueue) {
    writableFields()->rxReasonToQueue.swap(rxReasonToQueue);
  }

  ControlPlane* modify(std::shared_ptr<SwitchState>* state);

  bool operator==(const ControlPlane& controlPlane) const;
  bool operator!=(const ControlPlane& controlPlane) {
    return !(*this == controlPlane);
  }

private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};
}} // facebook::fboss
