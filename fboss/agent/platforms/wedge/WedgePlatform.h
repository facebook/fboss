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
#include "fboss/agent/platforms/wedge/WedgePortMapping.h"
#include "fboss/agent/types.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeI2CBusLock.h"

#include <boost/container/flat_map.hpp>
#include <folly/MacAddress.h>
#include <folly/Range.h>
#include <memory>
#include <unordered_map>

DECLARE_bool(enable_routes_in_host_table);

namespace facebook::fboss {

class BcmSwitch;
class WedgePort;
class PlatformProductInfo;

class WedgePlatform : public BcmPlatform, public StateObserver {
 public:
  explicit WedgePlatform(std::unique_ptr<PlatformProductInfo> productInfo);
  ~WedgePlatform() override;

  void initImpl() override;
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
  void onInitialConfigApplied(SwSwitch* sw) override {}

  bool canUseHostTableForHostRoutes() const override {
    return FLAGS_enable_routes_in_host_table;
  }
  WedgePort* getPort(PortID id) const;
  WedgePort* getPort(TransceiverID id) const;
  TransceiverIdxThrift getPortMapping(PortID port) const override;
  PlatformPort* getPlatformPort(PortID id) const override;
  BcmWarmBootHelper* getWarmBootHelper() override {
    return warmBootHelper_.get();
  }

  QsfpCache* getQsfpCache() const {
    return qsfpCache_.get();
  }

 protected:
  std::unique_ptr<WedgePortMapping> portMapping_{nullptr};

 private:
  // Forbidden copy constructor and assignment operator
  WedgePlatform(WedgePlatform const&) = delete;
  WedgePlatform& operator=(WedgePlatform const&) = delete;

  void initLocalMac();
  virtual void initLEDs();
  virtual std::map<std::string, std::string> loadConfig();

  virtual std::unique_ptr<BaseWedgeI2CBus> getI2CBus();

  virtual folly::ByteRange defaultLed0Code() = 0;
  virtual folly::ByteRange defaultLed1Code() = 0;

  std::unique_ptr<BcmSwitch> hw_;

  std::unique_ptr<AutoInitQsfpCache> qsfpCache_;
  std::unique_ptr<BcmWarmBootHelper> warmBootHelper_;
};

} // namespace facebook::fboss
