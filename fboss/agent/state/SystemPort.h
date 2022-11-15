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
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <optional>

namespace facebook::fboss {

struct SystemPortFields
    : public ThriftyFields<SystemPortFields, state::SystemPortFields> {
  explicit SystemPortFields(SystemPortID id) {
    auto& data = writableData();
    *data.portId() = id;
  }

  bool operator==(const SystemPortFields& other) const {
    return data() == other.data();
  }

  state::SystemPortFields toThrift() const override {
    return data();
  }

  template <typename Fn>
  void forEachChild(Fn /*fn*/) {}

  static SystemPortFields fromThrift(
      const state::SystemPortFields& systemPortThrift);
};

USE_THRIFT_COW(SystemPort);

class SystemPort
    : public ThriftStructNode<SystemPort, state::SystemPortFields> {
 public:
  using Base = ThriftStructNode<SystemPort, state::SystemPortFields>;
  using LegacyFields = SystemPortFields;
  explicit SystemPort(SystemPortID id) {
    set<switch_state_tags::portId>(static_cast<int64_t>(id));
  }
  SystemPortID getID() const {
    return static_cast<SystemPortID>(
        cref<switch_state_tags::portId>()->toThrift());
  }
  SwitchID getSwitchId() const {
    return static_cast<SwitchID>(
        cref<switch_state_tags::switchId>()->toThrift());
  }
  void setSwitchId(SwitchID swId) {
    set<switch_state_tags::switchId>(static_cast<int64_t>(swId));
  }
  std::string getPortName() const {
    return get<switch_state_tags::portName>()->toThrift();
  }
  void setPortName(const std::string& portName) {
    set<switch_state_tags::portName>(portName);
  }
  int64_t getCoreIndex() const {
    return cref<switch_state_tags::coreIndex>()->toThrift();
  }
  void setCoreIndex(int64_t coreIndex) {
    set<switch_state_tags::coreIndex>(coreIndex);
  }

  int64_t getCorePortIndex() const {
    return cref<switch_state_tags::corePortIndex>()->toThrift();
  }
  void setCorePortIndex(int64_t corePortIndex) {
    set<switch_state_tags::corePortIndex>(corePortIndex);
  }
  int64_t getSpeedMbps() const {
    return cref<switch_state_tags::speedMbps>()->toThrift();
  }
  void setSpeedMbps(int64_t speedMbps) {
    set<switch_state_tags::speedMbps>(speedMbps);
  }
  int64_t getNumVoqs() const {
    return cref<switch_state_tags::numVoqs>()->toThrift();
  }
  void setNumVoqs(int64_t numVoqs) {
    set<switch_state_tags::numVoqs>(numVoqs);
  }
  bool getEnabled() const {
    return cref<switch_state_tags::enabled>()->toThrift();
  }
  void setEnabled(bool enabled) {
    set<switch_state_tags::enabled>(enabled);
  }
  std::optional<std::string> getQosPolicy() const {
    if (const auto& policy = cref<switch_state_tags::qosPolicy>()) {
      return policy->toThrift();
    }
    return std::nullopt;
  }
  void setQosPolicy(const std::optional<std::string>& qosPolicy) {
    if (qosPolicy) {
      set<switch_state_tags::qosPolicy>(qosPolicy.value());
    } else {
      ref<switch_state_tags::qosPolicy>().reset();
    }
  }

  folly::dynamic toFollyDynamic() const override {
    auto fields = SystemPortFields::fromThrift(toThrift());
    return fields.toFollyDynamic();
  }

  folly::dynamic toFollyDynamicLegacy() const {
    return toFollyDynamic();
  }

  static std::shared_ptr<SystemPort> fromFollyDynamic(
      const folly::dynamic& dyn) {
    auto fields = SystemPortFields::fromFollyDynamic(dyn);
    return std::make_shared<SystemPort>(fields.toThrift());
  }

  static std::shared_ptr<SystemPort> fromFollyDynamicLegacy(
      const folly::dynamic& dyn) {
    return fromFollyDynamic(dyn);
  }

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
