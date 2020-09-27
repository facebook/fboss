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
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/StateObserver.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/platforms/common/PlatformProductInfo.h"
#include "fboss/agent/platforms/sai/SaiPlatformPort.h"
#include "fboss/agent/platforms/tests/utils/TestPlatformTypes.h"

#include "fboss/agent/hw/sai/api/SaiVersion.h"

#include <memory>
#include <vector>

extern "C" {
#include <sai.h>
#include <saistatus.h>
#include <saiswitch.h>
}

namespace facebook::fboss {

class SaiSwitch;
class HwSwitchWarmBootHelper;
class AutoInitQsfpCache;
class QsfpCache;

class SaiPlatform : public Platform, public StateObserver {
 public:
  explicit SaiPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<PlatformMapping> platformMapping);
  ~SaiPlatform() override;

  HwSwitch* getHwSwitch() const override;
  void onHwInitialized(SwSwitch* sw) override;
  void onInitialConfigApplied(SwSwitch* sw) override;
  std::unique_ptr<ThriftHandler> createHandler(SwSwitch* sw) override;
  TransceiverIdxThrift getPortMapping(PortID port, cfg::PortSpeed speed)
      const override;
  virtual std::optional<std::string> getPlatformAttribute(
      cfg::PlatformAttributes platformAttribute);
  virtual SaiPlatformPort* getPort(PortID id) const;
  PlatformPort* getPlatformPort(PortID port) const override;
  void initPorts() override;
  virtual std::string getHwConfig() = 0;
  std::string getHwConfigDumpFile();
  void generateHwConfigFile();
  virtual sai_service_method_table_t* getServiceMethodTable() const;
  void stop() override;
  HwSwitchWarmBootHelper* getWarmBootHelper();
  virtual uint32_t numLanesPerCore() const = 0;
  void stateUpdated(const StateDelta& delta) override;
  QsfpCache* getQsfpCache() const;

  virtual std::vector<PortID> getAllPortsInGroup(PortID portID) const = 0;
  virtual std::vector<FlexPortMode> getSupportedFlexPortModes() const = 0;
  /*
   * Get ids of all controlling ports
   */
  virtual std::vector<PortID> masterLogicalPortIds() const {
    // TODO make this pure virtual when we cook up a platform
    // for fake SAI
    return {};
  }

  PortID findPortID(cfg::PortSpeed speed, std::vector<uint32_t> lanes) const;

  bool supportsAddRemovePort() const override {
    return true;
  }

  virtual bool supportInterfaceType() const = 0;

  virtual std::optional<sai_port_interface_type_t> getInterfaceType(
      TransmitterTechnology transmitterTech,
      cfg::PortSpeed speed) const = 0;

  virtual bool isSerdesApiSupported() = 0;

  std::vector<phy::TxSettings> getPlatformPortTxSettings(
      PortID port,
      cfg::PortProfileID profile);

  std::vector<phy::RxSettings> getPlatformPortRxSettings(
      PortID port,
      cfg::PortProfileID profile);

  std::vector<SaiPlatformPort*> getPortsWithTransceiverID(
      TransceiverID id) const;

  virtual void initLEDs() = 0;

 private:
  void initImpl(uint32_t hwFeaturesDesired) override;
  void initSaiProfileValues();
  void updateQsfpCache(const StateDelta& delta);
  std::unique_ptr<SaiSwitch> saiSwitch_;
  std::unordered_map<PortID, std::unique_ptr<SaiPlatformPort>> portMapping_;
  std::unique_ptr<HwSwitchWarmBootHelper> wbHelper_;
  std::unique_ptr<AutoInitQsfpCache> qsfpCache_;
};

} // namespace facebook::fboss
