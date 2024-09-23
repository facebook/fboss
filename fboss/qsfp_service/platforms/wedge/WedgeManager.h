// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <boost/container/flat_map.hpp>

#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/i2c/gen-cpp2/i2c_controller_stats_types.h"
#include "fboss/lib/platforms/PlatformMode.h"
#include "fboss/lib/usb/WedgeI2CBus.h"
#include "fboss/qsfp_service/TransceiverManager.h"
#include "fboss/qsfp_service/fsdb/QsfpFsdbSyncManager.h"
#include "fboss/qsfp_service/platforms/wedge/QsfpRestClient.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeI2CBusLock.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeQsfp.h"

DECLARE_bool(override_program_iphy_ports_for_test);

namespace facebook::fboss {

class PhyManager;

class WedgeManager : public TransceiverManager {
 public:
  using TransceiverMap = std::map<int32_t, TransceiverInfo>;
  using PortMap = std::map<int32_t, PortStatus>;

  explicit WedgeManager(
      std::unique_ptr<TransceiverPlatformApi> api,
      std::unique_ptr<PlatformMapping> platformMapping,
      PlatformType type);
  ~WedgeManager() override;

  void getTransceiversInfo(
      TransceiverMap& info,
      std::unique_ptr<std::vector<int32_t>> ids) override;
  void getAllTransceiversValidationInfo(
      std::map<int32_t, std::string>& info,
      std::unique_ptr<std::vector<int32_t>> ids,
      bool getConfigString) override;
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
  void syncPorts(TransceiverMap& info, std::unique_ptr<PortMap> ports) override;

  PlatformType getPlatformType() const override {
    return platformType_;
  }

  int getNumQsfpModules() const override {
    return 16;
  }
  std::vector<TransceiverID> refreshTransceivers() override;
  void publishTransceiversToFsdb() override;

  int scanTransceiverPresence(
      std::unique_ptr<std::vector<int32_t>> ids) override;

  /* The function gets the i2c gets the i2c transaction stats. This class
   * will be inherited by platform specific class like Minipack16QManager from
   * where this function will be called. This function uses platform
   * specific I2c class routing to get these counters
   */
  std::vector<I2cControllerStats> getI2cControllerStats() const override {
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
  void getAndClearTransceiversModuleStatus(
      std::map<int32_t, ModuleStatus>& moduleStatusMap,
      std::unique_ptr<std::vector<int32_t>> ids) override;

  /*
   * This function will call PhyManager to create all the ExternalPhy objects
   */
  bool initExternalPhyMap(bool forceWarmboot = false) override;

  /*
   * Virtual function to program a PHY port on external PHY. This is a dummy
   * function here, it needs to be implemented by the platforms which support
   * external PHY and the PHY code is running in this qsfp_service process
   */
  void programXphyPort(PortID portId, cfg::PortProfileID portProfileId)
      override;

  void programXphyPortPrbs(
      PortID portID,
      phy::Side side,
      const phy::PortPrbsState& prbs);

  phy::PhyInfo getXphyInfo(PortID portID) override;

  phy::PortPrbsState getXphyPortPrbs(PortID portID, phy::Side side);

  void updateAllXphyPortsStats() override;

  virtual std::vector<PortID> getMacsecCapablePorts() const override;

  virtual std::string listHwObjects(
      std::vector<HwObjectType>& hwObjects,
      bool cached) const override;

  virtual bool getSdkState(std::string filename) const override;

  void triggerVdmStatsCapture(std::vector<int32_t>& ids) override;

  void setOverrideTcvrToPortAndProfileForTesting(
      std::optional<OverrideTcvrToPortAndProfile> overrideTcvrToPortAndProfile =
          std::nullopt) override;

  // Transceiver I2C Logging APIs
  size_t getI2cLogBufferCapacity(int32_t portId);
  std::pair<size_t, size_t> dumpTransceiverI2cLog(
      const std::string& portName) override;
  std::pair<size_t, size_t> dumpTransceiverI2cLog(int32_t portId) {
    return qsfpImpls_[portId]->dumpTransceiverI2cLog();
  }
  std::string getI2cLogName(int32_t portId) const {
    return FLAGS_qsfp_service_volatile_dir + "/i2cLog" +
        std::to_string(portId) + ".log";
  }

  virtual void publishPhyStateToFsdb(
      std::string&& portName,
      std::optional<phy::PhyState>&& newState) const override;
  virtual void publishPhyStatToFsdb(
      std::string&& portName,
      phy::PhyStats&& stat) const override;

  virtual void publishPortStatToFsdb(std::string&& portName, HwPortStats&& stat)
      const override;

  void loadConfig() override;

  void createQsfpToBmcSyncInterface();

  std::string getSwitchDataCenter() const {
    return dataCenter_;
  }

  std::string getSwitchRole() const {
    return hostnameScheme_;
  }

  QsfpToBmcSyncData getQsfpToBmcSyncData() const;

  std::string getQsfpToBmcSyncDataSerialized() const;

 protected:
  void initTransceiverMap() override;

  virtual std::unique_ptr<TransceiverI2CApi> getI2CBus() override;

  virtual TransceiverI2CApi* i2cBus() override {
    return wedgeI2cBus_.get();
  }

  void updateTransceiverMap();

  // thread safe handle to access bus
  std::unique_ptr<TransceiverI2CApi> wedgeI2cBus_;

  PlatformType platformType_;

  void updateTcvrStateInFsdb(
      TransceiverID tcvrID,
      facebook::fboss::TcvrState&& newState) override;

  void initQsfpImplMap();

 private:
  // Forbidden copy constructor and assignment operator
  WedgeManager(WedgeManager const&) = delete;
  WedgeManager& operator=(WedgeManager const&) = delete;

  using LockedTransceiversPtr = folly::Synchronized<
      std::map<TransceiverID, std::unique_ptr<Transceiver>>>::WLockedPtr;

  std::unique_ptr<QsfpFsdbSyncManager> fsdbSyncManager_;
  std::unique_ptr<QsfpRestClient> qsfpRestClient_;
  std::string dataCenter_{""};
  std::string hostnameScheme_{""};
  time_t nextOpticsToBmcSyncTime_{0};
  std::vector<std::unique_ptr<WedgeQsfp>> qsfpImpls_;
};
} // namespace facebook::fboss
