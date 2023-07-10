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
#include "fboss/agent/platforms/sai/SaiPlatformPort.h"
#include "fboss/agent/platforms/tests/utils/TestPlatformTypes.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/api/Types.h"

#include <memory>
#include <vector>

extern "C" {
#include <sai.h>
#include <saistatus.h>
#include <saiswitch.h>
}

DECLARE_string(hw_config_file);

namespace facebook::fboss {

class SaiSwitch;

class SaiPlatform : public Platform, public StateObserver {
 public:
  explicit SaiPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<PlatformMapping> platformMapping,
      folly::MacAddress localMac);
  ~SaiPlatform() override;

  HwSwitch* getHwSwitch() const override;
  void onHwInitialized(HwSwitchCallback* sw) override;
  void onInitialConfigApplied(HwSwitchCallback* sw) override;
  std::shared_ptr<apache::thrift::AsyncProcessorFactory> createHandler()
      override;
  TransceiverIdxThrift getPortMapping(PortID port, cfg::PortSpeed speed)
      const override;
  virtual SaiPlatformPort* getPort(PortID id) const;
  PlatformPort* getPlatformPort(PortID port) const override;
  void initPorts() override;
  virtual std::string getHwConfig() = 0;
  std::string getHwConfigDumpFile();
  void generateHwConfigFile();
  virtual sai_service_method_table_t* getServiceMethodTable() const;
  void stop() override;
  HwSwitchWarmBootHelper* getWarmBootHelper() override;
  void stateUpdated(const StateDelta& delta) override;

  virtual std::vector<PortID> getAllPortsInGroup(PortID portID) const = 0;
  virtual std::vector<FlexPortMode> getSupportedFlexPortModes() const = 0;
  /*
   * Get ids of all controlling ports
   */
  std::vector<PortID> masterLogicalPortIds() const {
    return masterLogicalPortIds_;
  }

  virtual PortID findPortID(
      cfg::PortSpeed speed,
      std::vector<uint32_t> lanes,
      PortSaiId portSaiId) const;

  bool supportsAddRemovePort() const override {
    return true;
  }

  virtual bool supportInterfaceType() const = 0;

  virtual std::optional<sai_port_interface_type_t> getInterfaceType(
      TransmitterTechnology transmitterTech,
      cfg::PortSpeed speed) const = 0;

  virtual bool isSerdesApiSupported() const = 0;

  std::vector<SaiPlatformPort*> getPortsWithTransceiverID(
      TransceiverID id) const;

  virtual void initLEDs() = 0;

  virtual SaiSwitchTraits::CreateAttributes getSwitchAttributes(
      bool mandatoryOnly,
      cfg::SwitchType switchType,
      std::optional<int64_t> switchId);

  uint32_t getDefaultMacAgingTime() const;

  virtual std::optional<SaiSwitchTraits::Attributes::AclFieldList>
  getAclFieldList() const {
    return std::nullopt;
  }

  /*
   * Based on the switch types: regular switch vs phy, some apis might not be
   * supported.
   * The default apis list for these two different asic types are just based
   * on our current experience. Eventually we will use getSupportedApiList()
   * to return current SaiPlatform supported api list. In most case, we can just
   * direcly reuse the default list. But user can also adjust the api list based
   * on specfic platform.
   */
  const std::set<sai_api_t>& getDefaultSwitchAsicSupportedApis() const;
  const std::set<sai_api_t>& getDefaultPhyAsicSupportedApis() const;
  virtual const std::set<sai_api_t>& getSupportedApiList() const;

  virtual const std::unordered_map<std::string, std::string>
  getSaiProfileVendorExtensionValues() const {
    return std::unordered_map<std::string, std::string>();
  }

 protected:
  std::unique_ptr<SaiSwitch> saiSwitch_;
  virtual void updatePorts(const StateDelta& delta);
  std::unordered_map<PortID, std::unique_ptr<SaiPlatformPort>> portMapping_;

 private:
  /*
   * Platform dependent internal {cpu, loopback} system ports. These must be
   * provided as part of switch init
   */
  virtual std::vector<sai_system_port_config_t> getInternalSystemPortConfig()
      const;
  void initImpl(uint32_t hwFeaturesDesired) override;
  void initSaiProfileValues();
  std::unique_ptr<HwSwitchWarmBootHelper> wbHelper_;
  // List of controlling ports on platform. Each of these then
  // have subports that can be used when using flex ports
  std::vector<PortID> masterLogicalPortIds_;
};

} // namespace facebook::fboss
