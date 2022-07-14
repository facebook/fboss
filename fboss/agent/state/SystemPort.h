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

class SystemPort : public ThriftyBaseT<
                       state::SystemPortFields,
                       SystemPort,
                       SystemPortFields> {
 public:
  SystemPortID getID() const {
    return SystemPortID(*getFields()->data().portId());
  }
  SwitchID getSwitchId() const {
    return SwitchID(*getFields()->data().switchId());
  }
  void setSwitchId(SwitchID swId) {
    writableFields()->writableData().switchId() = swId;
  }
  std::string getPortName() const {
    return *getFields()->data().portName();
  }
  void setPortName(const std::string& portName) {
    writableFields()->writableData().portName() = portName;
  }
  int64_t getCoreIndex() const {
    return *getFields()->data().coreIndex();
  }
  void setCoreIndex(int64_t coreIndex) {
    writableFields()->writableData().coreIndex() = coreIndex;
  }

  int64_t getCorePortIndex() const {
    return *getFields()->data().corePortIndex();
  }
  void setCorePortIndex(int64_t corePortIndex) {
    writableFields()->writableData().corePortIndex() = corePortIndex;
  }
  int64_t getSpeedMbps() const {
    return *getFields()->data().speedMbps();
  }
  void setSpeedMbps(int64_t speedMbps) {
    writableFields()->writableData().speedMbps() = speedMbps;
  }
  int64_t getNumVoqs() const {
    return *getFields()->data().numVoqs();
  }
  void setNumVoqs(int64_t numVoqs) {
    writableFields()->writableData().numVoqs() = numVoqs;
  }
  bool getEnabled() const {
    return *getFields()->data().enabled();
  }
  void setEnabled(bool enabled) {
    writableFields()->writableData().enabled() = enabled;
  }
  std::optional<std::string> getQosPolicy() const {
    return getFields()->data().qosPolicy().to_optional();
  }
  void setQosPolicy(const std::optional<std::string>& qosPolicy) {
    writableFields()->writableData().qosPolicy().from_optional(qosPolicy);
  }

 private:
  // Inherit the constructors required for clone()
  using ThriftyBaseT<state::SystemPortFields, SystemPort, SystemPortFields>::
      ThriftyBaseT;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
