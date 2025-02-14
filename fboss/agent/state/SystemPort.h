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

#include "fboss/agent/gen-cpp2/platform_config_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <optional>

namespace facebook::fboss {

USE_THRIFT_COW(SystemPort);

class SystemPort
    : public ThriftStructNode<SystemPort, state::SystemPortFields> {
 public:
  using Base = ThriftStructNode<SystemPort, state::SystemPortFields>;
  explicit SystemPort(
      SystemPortID id,
      std::optional<RemoteSystemPortType> remoteSystemPortType = std::nullopt) {
    set<ctrl_if_tags::portId>(static_cast<int64_t>(id));
    setRemoteSystemPortType(remoteSystemPortType);
  }
  SystemPortID getID() const {
    return static_cast<SystemPortID>(cref<ctrl_if_tags::portId>()->toThrift());
  }
  SwitchID getSwitchId() const {
    return static_cast<SwitchID>(cref<ctrl_if_tags::switchId>()->toThrift());
  }
  void setSwitchId(SwitchID swId) {
    set<ctrl_if_tags::switchId>(static_cast<int64_t>(swId));
  }
  std::string getName() const {
    return get<ctrl_if_tags::portName>()->toThrift();
  }
  void setName(const std::string& name) {
    set<ctrl_if_tags::portName>(name);
  }
  auto getPortQueues() const {
    return safe_cref<switch_state_tags::queues>();
  }
  void resetPortQueues(const QueueConfig& queues) {
    // TODO: change type to ThriftListNode
    std::vector<PortQueueFields> queuesThrift{};
    for (const auto& queue : queues) {
      queuesThrift.push_back(queue->toThrift());
    }
    set<switch_state_tags::queues>(std::move(queuesThrift));
  }
  int64_t getCoreIndex() const {
    return cref<ctrl_if_tags::coreIndex>()->toThrift();
  }
  void setCoreIndex(int64_t coreIndex) {
    set<ctrl_if_tags::coreIndex>(coreIndex);
  }

  int64_t getCorePortIndex() const {
    return cref<ctrl_if_tags::corePortIndex>()->toThrift();
  }
  void setCorePortIndex(int64_t corePortIndex) {
    set<ctrl_if_tags::corePortIndex>(corePortIndex);
  }
  int64_t getSpeedMbps() const {
    return cref<ctrl_if_tags::speedMbps>()->toThrift();
  }
  void setSpeedMbps(int64_t speedMbps) {
    set<ctrl_if_tags::speedMbps>(speedMbps);
  }
  int64_t getNumVoqs() const {
    return cref<ctrl_if_tags::numVoqs>()->toThrift();
  }
  void setNumVoqs(int64_t numVoqs) {
    set<ctrl_if_tags::numVoqs>(numVoqs);
  }
  std::optional<std::string> getQosPolicy() const {
    if (const auto& policy = cref<ctrl_if_tags::qosPolicy>()) {
      return policy->toThrift();
    }
    return std::nullopt;
  }
  void setQosPolicy(const std::optional<std::string>& qosPolicy) {
    if (qosPolicy) {
      set<ctrl_if_tags::qosPolicy>(qosPolicy.value());
    } else {
      ref<ctrl_if_tags::qosPolicy>().reset();
    }
  }

  std::optional<RemoteSystemPortType> getRemoteSystemPortType() const {
    if (auto remoteSystemPortType =
            cref<ctrl_if_tags::remoteSystemPortType>()) {
      return remoteSystemPortType->cref();
    }
    return std::nullopt;
  }

  void setRemoteSystemPortType(
      const std::optional<RemoteSystemPortType>& remoteSystemPortType =
          std::nullopt) {
    if (remoteSystemPortType) {
      set<ctrl_if_tags::remoteSystemPortType>(remoteSystemPortType.value());
    } else {
      ref<ctrl_if_tags::remoteSystemPortType>().reset();
    }
  }

  std::optional<LivenessStatus> getRemoteLivenessStatus() const {
    if (auto remoteSystemPortLivenessStatus =
            cref<ctrl_if_tags::remoteSystemPortLivenessStatus>()) {
      return remoteSystemPortLivenessStatus->cref();
    }
    return std::nullopt;
  }

  void setRemoteLivenessStatus(
      const std::optional<LivenessStatus>& remoteSystemPortLivenessStatus =
          std::nullopt) {
    if (remoteSystemPortLivenessStatus) {
      set<ctrl_if_tags::remoteSystemPortLivenessStatus>(
          remoteSystemPortLivenessStatus.value());
    } else {
      ref<ctrl_if_tags::remoteSystemPortLivenessStatus>().reset();
    }
  }

  void setScope(const cfg::Scope& scope) {
    set<ctrl_if_tags::scope>(scope);
  }

  cfg::Scope getScope() const {
    return cref<ctrl_if_tags::scope>()->cref();
  }

  bool isStatic() const {
    return getRemoteSystemPortType().has_value() &&
        getRemoteSystemPortType().value() == RemoteSystemPortType::STATIC_ENTRY;
  }

  std::optional<bool> getShelDestinationEnabled() const {
    if (auto shelDestinationEnabled =
            cref<ctrl_if_tags::shelDestinationEnabled>()) {
      return shelDestinationEnabled->cref();
    };
    return std::nullopt;
  }
  void setShelDestinationEnabled(std::optional<bool> shelDestinationEnabled) {
    if (!shelDestinationEnabled.has_value()) {
      ref<ctrl_if_tags::shelDestinationEnabled>().reset();
    } else {
      set<ctrl_if_tags::shelDestinationEnabled>(shelDestinationEnabled.value());
    }
  }

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
