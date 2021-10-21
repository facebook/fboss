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

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/StateObserver.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/types.h"
#include "fboss/lib/phy/PhyInterfaceHandler.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeI2CBusLock.h"

#include <boost/container/flat_map.hpp>
#include <folly/MacAddress.h>
#include <folly/Range.h>
#include <memory>
#include <unordered_map>

DECLARE_bool(enable_routes_in_host_table);
DECLARE_string(volatile_state_dir);
DECLARE_string(persistent_state_dir);

namespace facebook::fboss {

class BcmSwitch;
class PlatformProductInfo;
class WedgePort;
class WedgePortMapping;

class WedgePlatform : public BcmPlatform, public StateObserver {
 public:
  explicit WedgePlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<PlatformMapping> platformMapping,
      folly::MacAddress localMac);
  ~WedgePlatform() override;

  void initPorts() override;
  BcmPlatformPortMap getPlatformPortMap() override;

  void stop() override;

  void stateUpdated(const StateDelta& /*delta*/) override;

  virtual std::unique_ptr<WedgePortMapping> createPortMapping() = 0;

  HwSwitch* getHwSwitch() const override;
  void onHwInitialized(SwSwitch* sw) override;
  std::unique_ptr<ThriftHandler> createHandler(SwSwitch* sw) override;

  std::string getVolatileStateDir() const override;
  std::string getPersistentStateDir() const override;

  void onUnitCreate(int unit) override;
  void onUnitAttach(int unit) override;
  void preWarmbootStateApplied() override;
  void onInitialConfigApplied(SwSwitch* sw) override {}

  bool canUseHostTableForHostRoutes() const override {
    return FLAGS_enable_routes_in_host_table;
  }
  WedgePort* getPort(PortID id) const;
  std::vector<WedgePort*> getPortsByTransceiverID(TransceiverID id) const;
  TransceiverIdxThrift getPortMapping(PortID port, cfg::PortSpeed /* speed */)
      const override;
  PlatformPort* getPlatformPort(PortID id) const override;
  BcmWarmBootHelper* getWarmBootHelper() override {
    return warmBootHelper_.get();
  }

  QsfpCache* getQsfpCache() const override {
    return qsfpCache_.get();
  }

  PhyInterfaceHandler* getPhyInterfaceHandler() override {
    return phyInterfaceHandler_.get();
  }

 protected:
  std::unique_ptr<WedgePortMapping> portMapping_;

  // The platform contains the top level Phy Interface handler object. The
  // Agent can use this to access the Phy functions. The Phy interface handler
  // will either have local Phy Manager object for Phy function or it will
  // make thrift call to port service for the Phy function
  std::unique_ptr<PhyInterfaceHandler> phyInterfaceHandler_;

  void initImpl(uint32_t hwFeaturesDesired) override;

  virtual void updateQsfpCache(const StateDelta& delta);

  virtual void updatePorts(const StateDelta& delta);

 private:
  // Forbidden copy constructor and assignment operator
  WedgePlatform(WedgePlatform const&) = delete;
  WedgePlatform& operator=(WedgePlatform const&) = delete;

  void initLocalMac();
  virtual void initLEDs();
  virtual std::map<std::string, std::string> loadConfig();
  std::string loadYamlConfig();

  virtual std::unique_ptr<BaseWedgeI2CBus> getI2CBus();

  virtual folly::ByteRange defaultLed0Code() = 0;
  virtual folly::ByteRange defaultLed1Code() = 0;

  std::unique_ptr<BcmSwitch> hw_;

  std::unique_ptr<AutoInitQsfpCache> qsfpCache_;
  std::unique_ptr<BcmWarmBootHelper> warmBootHelper_;
};

} // namespace facebook::fboss
