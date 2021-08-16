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

namespace facebook::fboss {

class SwitchState;

struct SwitchSettingsFields {
  template <typename Fn>
  void forEachChild(Fn /*fn*/) {}

  folly::dynamic toFollyDynamic() const;
  static SwitchSettingsFields fromFollyDynamic(const folly::dynamic& json);

  cfg::L2LearningMode l2LearningMode = cfg::L2LearningMode::HARDWARE;
  bool qcmEnable = false;
  bool ptpTcEnable = false;
  uint32_t l2AgeTimerSeconds{300};
  uint32_t maxRouteCounterIDs{0};
  std::vector<std::pair<VlanID, folly::IPAddress>> blockNeighbors;
};

/*
 * SwitchSettings stores state about path settings of traffic to userver CPU
 * on the switch.
 */
class SwitchSettings : public NodeBaseT<SwitchSettings, SwitchSettingsFields> {
 public:
  static std::shared_ptr<SwitchSettings> fromFollyDynamic(
      const folly::dynamic& json) {
    const auto& fields = SwitchSettingsFields::fromFollyDynamic(json);
    return std::make_shared<SwitchSettings>(fields);
  }

  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamic();
  }

  cfg::L2LearningMode getL2LearningMode() const {
    return getFields()->l2LearningMode;
  }

  void setL2LearningMode(cfg::L2LearningMode l2LearningMode) {
    writableFields()->l2LearningMode = l2LearningMode;
  }

  bool isQcmEnable() const {
    return getFields()->qcmEnable;
  }

  void setQcmEnable(const bool enable) {
    writableFields()->qcmEnable = enable;
  }

  bool isPtpTcEnable() const {
    return getFields()->ptpTcEnable;
  }

  void setPtpTcEnable(const bool enable) {
    writableFields()->ptpTcEnable = enable;
  }

  uint32_t getL2AgeTimerSeconds() const {
    return getFields()->l2AgeTimerSeconds;
  }

  void setL2AgeTimerSeconds(uint32_t val) {
    writableFields()->l2AgeTimerSeconds = val;
  }

  void setMaxRouteCounterIDs(uint32_t numCounterIDs) {
    writableFields()->maxRouteCounterIDs = numCounterIDs;
  }

  uint32_t getMaxRouteCounterIDs() {
    return getFields()->maxRouteCounterIDs;
  }

  std::vector<std::pair<VlanID, folly::IPAddress>> getBlockNeighbors() const {
    return getFields()->blockNeighbors;
  }

  void setBlockNeighbors(
      const std::vector<std::pair<VlanID, folly::IPAddress>>& blockNeighbors) {
    writableFields()->blockNeighbors = blockNeighbors;
  }

  SwitchSettings* modify(std::shared_ptr<SwitchState>* state);

  bool operator==(const SwitchSettings& switchSettings) const;
  bool operator!=(const SwitchSettings& switchSettings) {
    return !(*this == switchSettings);
  }

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
