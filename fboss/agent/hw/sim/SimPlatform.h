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
  void onHwInitialized(SwSwitch* sw) override;
  void onInitialConfigApplied(SwSwitch* sw) override;
  void stop() override;

  std::unique_ptr<ThriftHandler> createHandler(SwSwitch* sw) override;

  std::string getVolatileStateDir() const override;
  std::string getPersistentStateDir() const override;
  TransceiverIdxThrift getPortMapping(
      PortID /* unused */,
      cfg::PortSpeed /* speed */) const override {
    return TransceiverIdxThrift();
  }
  PlatformPort* getPlatformPort(PortID id) const override;

  HwAsic* getAsic() const override {
    throw std::runtime_error("getAsic not implemented for SimPlatform");
  }

  void initPorts() override;
  QsfpCache* getQsfpCache() const override {
    return nullptr;
  }
  HwSwitchWarmBootHelper* getWarmBootHelper() override {
    return nullptr;
  }

 private:
  void setupAsic(
      cfg::SwitchType /*switchType*/,
      std::optional<int64_t> /*switchId*/,
      std::optional<cfg::Range64> /*systemPortRange*/,
      folly::MacAddress& /*mac*/) override {
    // noop - no asic implemented
  }
  // Forbidden copy constructor and assignment operator
  SimPlatform(SimPlatform const&) = delete;
  SimPlatform& operator=(SimPlatform const&) = delete;

  void initImpl(uint32_t /*hwFeaturesDesired*/) override {}

  std::unique_ptr<SimSwitch> hw_;
  uint32_t numPorts_;
  std::unordered_map<PortID, std::unique_ptr<SimPlatformPort>> portMapping_;
};

} // namespace facebook::fboss
