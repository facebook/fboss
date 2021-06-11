// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <boost/container/flat_map.hpp>

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/platforms/common/PlatformMode.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/i2c/gen-cpp2/i2c_controller_stats_types.h"
#include "fboss/lib/usb/WedgeI2CBus.h"
#include "fboss/qsfp_service/QsfpConfig.h"
#include "fboss/qsfp_service/TransceiverManager.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeI2CBusLock.h"

DECLARE_string(warmboot_dir);

namespace facebook::fboss {

class PhyManager;

class WedgeManager : public TransceiverManager {
 public:
  using TransceiverMap = std::map<int32_t, TransceiverInfo>;
  using PortMap = std::map<int32_t, PortStatus>;

  explicit WedgeManager(
      std::unique_ptr<TransceiverPlatformApi> api,
      std::unique_ptr<PlatformMapping> platformMapping,
      PlatformMode mode);
  ~WedgeManager() override {}

  void initTransceiverMap() override;
  void getTransceiversInfo(
      TransceiverMap& info,
      std::unique_ptr<std::vector<int32_t>> ids) override;
  void getTransceiversRawDOMData(
      std::map<int32_t, RawDOMData>& info,
      std::unique_ptr<std::vector<int32_t>> ids) override;
  void getTransceiversDOMDataUnion(
      std::map<int32_t, DOMDataUnion>& info,
      std::unique_ptr<std::vector<int32_t>> ids) override;
  void readTransceiverRegister(
      std::map<int32_t, ReadResponse>& response,
      std::unique_ptr<ReadRequest> request) override;
  void writeTransceiverRegister(
      std::map<int32_t, WriteResponse>& response,
      std::unique_ptr<WriteRequest> request) override;
  void customizeTransceiver(int32_t idx, cfg::PortSpeed speed) override;
  void syncPorts(TransceiverMap& info, std::unique_ptr<PortMap> ports) override;

  PlatformMode getPlatformMode() const override {
    return platformMode_;
  }

  int getNumQsfpModules() override {
    return 16;
  }
  int numPortsPerTransceiver() override {
    // most of our platforms specify a port per channel in a qsfp
    return 4;
  }
  void refreshTransceivers() override;

  int scanTransceiverPresence(
      std::unique_ptr<std::vector<int32_t>> ids) override;

  /* The function gets the i2c gets the i2c transaction stats. This class
   * will be inherited by platform specific class like Minipack16QManager from
   * where this function will be called. This function uses platform
   * specific I2c class routing to get these counters
   */
  std::vector<std::reference_wrapper<const I2cControllerStats>>
  getI2cControllerStats() const override {
    return wedgeI2cBus_->getI2cControllerStats();
  }

  /* Get the i2c transaction counters from TranscieverManager base class
   * and update to fbagent. The TransceieverManager base class is inherited
   * by platform speficic Transaceiver Manager class like WedgeQManager.
   * That class has the function to get the I2c transaction status
   */
  void publishI2cTransactionStats() override;

  /*
   * This is introduced mainly due to the mismatch of ODS reporting frequency
   * and the interval of us reading transceiver data. Some of the clear on read
   * information may be lost in this process and not being captured in the ODS
   * time series. This would bring difficulty in root cause link issues. Thus
   * here we provide a way of read and clear the data for the purpose of ODS
   * data reporting.
   */
  void getAndClearTransceiversSignalFlags(
      std::map<int32_t, SignalFlags>& signalFlagsMap,
      std::unique_ptr<std::vector<int32_t>> ids) override;
  void getAndClearTransceiversMediaSignals(
      std::map<int32_t, std::map<int, MediaLaneSignals>>& mediaSignalsMap,
      std::unique_ptr<std::vector<int32_t>> ids) override;

  // This function will bring all the transceivers out of reset, making use
  // of the specific implementation from each platform. Platforms that bring
  // transceiver out of reset by default will stay no op.
  virtual void clearAllTransceiverReset();

  // This function will trigger a hard reset on the specific transceiver, making
  // use of the specific implementation from each platform.
  virtual void triggerQsfpHardReset(int idx);

  /*
   * This function takes the portId, port profile id and creates phy port
   * config using platform mapping.
   */
  std::optional<phy::PhyPortConfig> getPhyPortConfigValues(
      int32_t portId,
      cfg::PortProfileID portProfileId);

  /*
   * This function will call PhyManager to create all the ExternalPhy objects
   */
  bool initExternalPhyMap() override {
    // TODO(rajank): To  implement this function for each platform
    return true;
  }

  /*
   * Virtual function to program a PHY port on external PHY. This is a dummy
   * function here, it needs to be implemented by the platforms which support
   * external PHY and the PHY code is running in this qsfp_service process
   */
  void programXphyPort(int32_t portId, cfg::PortProfileID portProfileId)
      override;

  PhyManager* getPhyManager() override {
    return phyManager_.get();
  }
  const AgentConfig* getAgentConfig() const {
    return agentConfig_.get();
  }

  // Function to convert port name string to software port id
  std::optional<int> getSwPortByPortName(const std::string& portName);

  virtual bool canWarmboot() const {
    return !forceColdBoot_;
  }

 protected:
  virtual std::unique_ptr<TransceiverI2CApi> getI2CBus();
  void updateTransceiverMap();

  void setPhyManager(std::unique_ptr<PhyManager> phyManager) {
    phyManager_ = std::move(phyManager);
  }

  // thread safe handle to access bus
  std::unique_ptr<TransceiverI2CApi> wedgeI2cBus_;

  std::unique_ptr<AgentConfig> agentConfig_;
  std::unique_ptr<QsfpConfig> qsfpConfig_;

  folly::Synchronized<std::map<TransceiverID, std::map<uint32_t, PortStatus>>>
      ports_;

  PlatformMode platformMode_;

  // For platforms that needs to program xphy
  std::unique_ptr<PhyManager> phyManager_;

 private:
  // Forbidden copy constructor and assignment operator
  WedgeManager(WedgeManager const&) = delete;
  WedgeManager& operator=(WedgeManager const&) = delete;

  void loadConfig() override;

  std::map<std::string, int> portNameToSwPort_;
  bool forceColdBoot_{false};
};
} // namespace facebook::fboss
