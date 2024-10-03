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

#include "fboss/agent/Platform.h"

DECLARE_string(volatile_state_dir);
DECLARE_string(persistent_state_dir);

namespace facebook::fboss {

class SimSwitch;
class SimPlatformPort;

class SimPlatform : public Platform {
 public:
  SimPlatform(folly::MacAddress mac, uint32_t numPorts);
  ~SimPlatform() override;

  HwSwitch* getHwSwitch() const override;
  void onHwInitialized(HwSwitchCallback* sw) override;
  void stateChanged(const StateDelta& /*delta*/) override {}

  std::shared_ptr<apache::thrift::AsyncProcessorFactory> createHandler()
      override {
    return nullptr;
  }

  TransceiverIdxThrift getPortMapping(
      PortID /* unused */,
      cfg::PortProfileID /* profileID */) const override {
    return TransceiverIdxThrift();
  }
  PlatformPort* getPlatformPort(PortID id) const override;

  HwAsic* getAsic() const override {
    throw std::runtime_error("getAsic not implemented for SimPlatform");
  }

  void initPorts() override;
  HwSwitchWarmBootHelper* getWarmBootHelper() override {
    return nullptr;
  }

  const AgentDirectoryUtil* getDirectoryUtil() const override {
    return agentDirUtil_.get();
  }

 private:
  void setupAsic(
      cfg::SwitchType /*switchType*/,
      std::optional<int64_t> /*switchId*/,
      int16_t /*switchIndex*/,
      std::optional<cfg::Range64> /*systemPortRange*/,
      folly::MacAddress& /*mac*/,
      std::optional<HwAsic::FabricNodeRole> /*fabricNodeRole) */
      ) override {
    // noop - no asic implemented
  }
  // Forbidden copy constructor and assignment operator
  SimPlatform(SimPlatform const&) = delete;
  SimPlatform& operator=(SimPlatform const&) = delete;

  void initImpl(uint32_t /*hwFeaturesDesired*/) override {}

  std::unique_ptr<SimSwitch> hw_;
  uint32_t numPorts_;
  std::unordered_map<PortID, std::unique_ptr<SimPlatformPort>> portMapping_;
  std::unique_ptr<AgentDirectoryUtil> agentDirUtil_;
};

} // namespace facebook::fboss
