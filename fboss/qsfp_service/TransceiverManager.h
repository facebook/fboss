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
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/types.h"
#include "fboss/lib/i2c/gen-cpp2/i2c_controller_stats_types.h"
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/lib/platforms/PlatformMode.h"
#include "fboss/lib/usb/TransceiverPlatformApi.h"
#include "fboss/qsfp_service/QsfpConfig.h"
#include "fboss/qsfp_service/TransceiverStateMachineUpdate.h"
#include "fboss/qsfp_service/module/Transceiver.h"

#include <folly/IntrusiveList.h>
#include <folly/SpinLock.h>
#include <folly/Synchronized.h>
#include <map>
#include <vector>

namespace facebook {
namespace fboss {
class TransceiverManager {
  using PortNameMap = std::map<std::string, int32_t>;
  using PortGroups = std::map<int32_t, std::set<cfg::Port>>;

 public:
  explicit TransceiverManager(
      std::unique_ptr<TransceiverPlatformApi> api,
      std::unique_ptr<PlatformMapping> platformMapping);
  virtual ~TransceiverManager();
  virtual void initTransceiverMap() = 0;
  virtual void getTransceiversInfo(
      std::map<int32_t, TransceiverInfo>& info,
      std::unique_ptr<std::vector<int32_t>> ids) = 0;
  virtual void getTransceiversRawDOMData(
      std::map<int32_t, RawDOMData>& info,
      std::unique_ptr<std::vector<int32_t>> ids) = 0;
  virtual void getTransceiversDOMDataUnion(
      std::map<int32_t, DOMDataUnion>& info,
      std::unique_ptr<std::vector<int32_t>> ids) = 0;
  virtual void readTransceiverRegister(
      std::map<int32_t, ReadResponse>& response,
      std::unique_ptr<ReadRequest> request) = 0;
  virtual void writeTransceiverRegister(
      std::map<int32_t, WriteResponse>& response,
      std::unique_ptr<WriteRequest> request) = 0;
  virtual void customizeTransceiver(int32_t idx, cfg::PortSpeed speed) = 0;
  virtual void syncPorts(
      std::map<int32_t, TransceiverInfo>& info,
      std::unique_ptr<std::map<int32_t, PortStatus>> ports) = 0;

  virtual PlatformMode getPlatformMode() const = 0;

  bool isValidTransceiver(int32_t id) {
    return id < getNumQsfpModules() && id >= 0;
  }
  virtual int getNumQsfpModules() = 0;
  virtual std::vector<TransceiverID> refreshTransceivers() = 0;
  virtual int scanTransceiverPresence(
      std::unique_ptr<std::vector<int32_t>> ids) = 0;
  virtual int numPortsPerTransceiver() = 0;
  /*
   * A function take a parameter representing number of seconds,
   * adding it to the time point of now and assign it to
   * pauseRemediationUntil_, which is a time point until when
   * the remediation of module will be paused.
   */
  void setPauseRemediation(int32_t timeout) {
    pauseRemediationUntil_ = std::time(nullptr) + timeout;
  }
  time_t getPauseRemediationUntil() {
    return pauseRemediationUntil_;
  }

  /* Virtual function to return the i2c transactions stats in a platform.
   * This will be overridden by derived classes which are platform specific
   * and has the platform specific implementation for this counter
   */
  virtual std::vector<std::reference_wrapper<const I2cControllerStats>>
  getI2cControllerStats() const = 0;

  /* Virtual function to update the I2c transaction stats to the ServiceData
   * object from where it will get picked up by FbAgent.
   * Implementation - The TransceieverManager base class is inherited
   * by platform speficic Transaceiver Manager class like WedgeManager.
   * That class has the function to get the I2c transaction status
   */
  virtual void publishI2cTransactionStats() = 0;

  /*
   * Virtual functions to get the cached transceiver signal flags, media lane
   * signals, and module status flags, and clear the cached data. This is
   * introduced mainly due to the mismatch of ODS reporting frequency and the
   * interval of us reading transceiver data. Some of the clear on read
   * information may be lost in this process and not being captured in the ODS
   * time series. This would bring difficulty in root cause link issues. Thus
   * here we provide a way of read and clear the data for the purpose of ODS
   * data reporting.
   */
  virtual void getAndClearTransceiversSignalFlags(
      std::map<int32_t, SignalFlags>& signalFlagsMap,
      std::unique_ptr<std::vector<int32_t>> ids) = 0;
  virtual void getAndClearTransceiversMediaSignals(
      std::map<int32_t, std::map<int, MediaLaneSignals>>& mediaSignalsMap,
      std::unique_ptr<std::vector<int32_t>> ids) = 0;
  virtual void getAndClearTransceiversModuleStatus(
      std::map<int32_t, ModuleStatus>& moduleStatusMap,
      std::unique_ptr<std::vector<int32_t>> ids) = 0;

  TransceiverPlatformApi* getQsfpPlatformApi() {
    return qsfpPlatApi_.get();
  }

  const PlatformMapping* getPlatformMapping() const {
    return platformMapping_.get();
  }
  /*
   * Virtual function to initialize all the Phy in the system
   */
  virtual bool initExternalPhyMap() = 0;

  PhyManager* getPhyManager() {
    return phyManager_.get();
  }

  /*
   * Virtual function to program a PHY port on external PHY. This function
   * needs to be implemented by the platforms which support external PHY
   * and the PHY code is running in this qsfp_service process
   */
  virtual void programXphyPort(
      PortID portId,
      cfg::PortProfileID portProfileId) = 0;

  virtual phy::PhyInfo getXphyInfo(PortID portId) = 0;

  virtual void updateAllXphyPortsStats() = 0;

  const PortGroups& getModuleToPortMap() const {
    return portGroupMap_;
  }

  const PortNameMap& getPortNameToModuleMap() const;

  const QsfpConfig* getQsfpConfig() const {
    return qsfpConfig_.get();
  };
  virtual std::vector<PortID> getMacsecCapablePorts() const = 0;

  virtual std::string listHwObjects(
      std::vector<HwObjectType>& hwObjects,
      bool cached) const = 0;

  virtual bool getSdkState(std::string filename) const = 0;

  // Returns the interface names for a given transceiverId
  // Returns empty list when there is no corresponding name for a given
  // transceiver ID
  const std::set<std::string> getPortNames(TransceiverID tcvrId) const;

  // Returns the first interface name found for the given transceiverId
  // returns empty string when there is no name found
  const std::string getPortName(TransceiverID tcvrId) const;

  // Since all the transceiver events need to have a proper order for the
  // correct port programming, we should always wait for the update results
  // before moving on to the next operations.
  void updateStateBlocking(
      TransceiverID id,
      TransceiverStateMachineEvent event);
  std::shared_ptr<BlockingTransceiverStateMachineUpdateResult>
  updateStateBlockingWithoutWait(
      TransceiverID id,
      TransceiverStateMachineEvent event);

  TransceiverStateMachineState getCurrentState(TransceiverID id) const;

  bool getNeedResetDataPath(TransceiverID id) const;

  // ========== Public functions for TransceiverStateMachine ==========
  // This refresh TransceiverStateMachine functions will handle all state
  // machine updates.
  void refreshStateMachines();

  void programInternalPhyPorts(TransceiverID id);

  void programExternalPhyPorts(TransceiverID id);

  void programTransceiver(TransceiverID id);

  bool areAllPortsDown(TransceiverID id) const noexcept;

  bool tryRemediateTransceiver(TransceiverID id);

  bool supportRemediateTransceiver(TransceiverID id);

  void markLastDownTime(TransceiverID id) noexcept;
  // ========== Public functions fo TransceiverStateMachine ==========

  // A struct to keep track of the software port profile and status
  struct TransceiverPortInfo {
    cfg::PortProfileID profile;
    std::optional<PortStatus> status;
  };
  std::unordered_map<PortID, TransceiverPortInfo>
  getProgrammedIphyPortToPortInfo(TransceiverID id) const;

  std::unordered_map<PortID, cfg::PortProfileID>
  getOverrideProgrammedIphyPortAndProfileForTest(TransceiverID id) const;

  // TEST ONLY
  void setOverrideAgentPortStatusForTesting(
      bool up,
      bool enabled,
      bool clearOnly = false);
  void setOverrideAgentConfigAppliedInfoForTesting(
      std::optional<ConfigAppliedInfo> configAppliedInfo);

  // If the transceiver doesn't exit, it will still return a TransceiverInfo
  // with present filed is false.
  TransceiverInfo getTransceiverInfo(TransceiverID id);

  // Function to convert port name string to software port id
  std::optional<PortID> getPortIDByPortName(const std::string& portName);

  virtual void triggerVdmStatsCapture(std::vector<int32_t>& ids) = 0;

  void publishLinkSnapshots(std::string portName);

  time_t getLastDownTime(TransceiverID id) const;

 protected:
  virtual void loadConfig() = 0;

  void setPhyManager(std::unique_ptr<PhyManager> phyManager) {
    phyManager_ = std::move(phyManager);
  }

  // Update the cached PortStatus of TransceiverToPortInfo based on the input
  // This will only change the active state of the state machine.
  void updateTransceiverActiveState(
      const std::set<TransceiverID>& tcvrs,
      const std::map<int32_t, PortStatus>& portStatus) noexcept;

  void publishLinkSnapshots(PortID portID);

  std::optional<TransceiverID> getTransceiverID(PortID id);

  // An override of programmed iphy ports.
  // Due to hw_test won't be able to get wedge_agent running, this override
  // map will mimic the return of programmed iphy ports based on transceiver.
  // NOTE: Only use in test
  using OverrideTcvrToPortAndProfile = std::unordered_map<
      TransceiverID,
      std::unordered_map<PortID, cfg::PortProfileID>>;
  virtual void setOverrideTcvrToPortAndProfileForTest() = 0;
  OverrideTcvrToPortAndProfile overrideTcvrToPortAndProfileForTest_;

  folly::Synchronized<std::map<TransceiverID, std::unique_ptr<Transceiver>>>
      transceivers_;
  /* This variable stores the TransceiverPlatformApi object for controlling
   * the QSFP devies on board. This handle is populated from this class
   * constructor
   */
  std::unique_ptr<TransceiverPlatformApi> qsfpPlatApi_;
  /* This variable stores the PlatformMapping object which has a map for all
   * the components connected on different ports. This handle is populated
   * from this class constructor
   */
  const std::unique_ptr<const PlatformMapping> platformMapping_;
  // A time point until when the remediation of module will be paused.
  // Before reaching that time point, the module is paused
  // and it will resume once the time is reached.
  time_t pauseRemediationUntil_{0};

  mutable PortNameMap portNameToModule_;
  PortGroups portGroupMap_;
  std::unique_ptr<QsfpConfig> qsfpConfig_;
  // For platforms that needs to program xphy
  std::unique_ptr<PhyManager> phyManager_;

  // Use the following map to cache the static mapping so that we don't have
  // to search from PlatformMapping again and again
  std::unordered_map<std::string, PortID> portNameToPortID_;
  struct SwPortInfo {
    std::optional<TransceiverID> tcvrID;
    std::string name;
  };
  std::unordered_map<PortID, SwPortInfo> portToSwPortInfo_;

 private:
  // Forbidden copy constructor and assignment operator
  TransceiverManager(TransceiverManager const&) = delete;
  TransceiverManager& operator=(TransceiverManager const&) = delete;

  using BlockingStateUpdateResultList =
      std::vector<std::shared_ptr<BlockingTransceiverStateMachineUpdateResult>>;
  void waitForAllBlockingStateUpdateDone(
      const BlockingStateUpdateResultList& results);

  /*
   * This is the private class to capture all information a
   * TransceiverStateMachine needs
   * A Synchronized state_machine to keep track of the state
   * thread and EventBase so that we can operate multiple different transceivers
   * StateMachine update at the same time and also better starting and
   * terminating these threads.
   */
  class TransceiverStateMachineHelper {
   public:
    TransceiverStateMachineHelper(
        TransceiverManager* tcvrMgrPtr,
        TransceiverID tcvrID);

    void startThread();
    void stopThread();
    folly::EventBase* getEventBase() const {
      return updateEventBase_.get();
    }
    folly::Synchronized<state_machine<TransceiverStateMachine>>&
    getStateMachine() {
      return stateMachine_;
    }

   private:
    TransceiverID tcvrID_;
    folly::Synchronized<state_machine<TransceiverStateMachine>> stateMachine_;
    // Can't use ScopedEventBaseThread as it won't work well with
    // handcrafted HeaderClientChannel client instead of servicerouter client
    std::unique_ptr<std::thread> updateThread_;
    std::unique_ptr<folly::EventBase> updateEventBase_;
  };

  using TransceiverToStateMachineHelper = std::unordered_map<
      TransceiverID,
      std::unique_ptr<TransceiverStateMachineHelper>>;
  TransceiverToStateMachineHelper setupTransceiverToStateMachineHelper();

  using TransceiverToPortInfo = std::unordered_map<
      TransceiverID,
      std::unique_ptr<folly::Synchronized<
          std::unordered_map<PortID, TransceiverPortInfo>>>>;
  TransceiverToPortInfo setupTransceiverToPortInfo();

  void startThreads();
  void stopThreads();
  void threadLoop(folly::StringPiece name, folly::EventBase* eventBase);

  /**
   * Schedule an update to the switch state.
   *
   *  @param  update
              The update to be enqueued
   *  @return bool whether the update was queued or not
   * This schedules the specified TransceiverStateMachineUpdate to be invoked
   * in the update thread in order to update the TransceiverStateMachine.
   *
   */
  bool updateState(std::unique_ptr<TransceiverStateMachineUpdate> update);

  static void handlePendingUpdatesHelper(TransceiverManager* mgr);
  void handlePendingUpdates();

  // Check whether iphy/xphy/transceiver programmed is done. If not, then
  // trigger the corresponding program event to program the component.
  // Return the list of transceivers that have programming events
  std::vector<TransceiverID> triggerProgrammingEvents();

  void triggerAgentConfigChangeEvent(
      const std::vector<TransceiverID>& transceivers);

  // Update the cached PortStatus of TransceiverToPortInfo using wedge_agent
  // getPortStatus() results
  void updateTransceiverPortStatus() noexcept;

  std::vector<PortID> getAllPlatformPorts(TransceiverID tcvrID) const;

  std::vector<TransceiverID> getPresentTransceivers() const;

  // Check whether the specified stableTcvrs need remediation and then trigger
  // the remediation events to remediate such transceivers.
  void triggerRemediateEvents(const std::vector<TransceiverID>& stableTcvrs);

  // TEST ONLY
  // This private map is an override of agent getPortStatus()
  std::map<int32_t, PortStatus> overrideAgentPortStatusForTesting_;
  // This ConfigAppliedInfo is an override of agent getConfigAppliedInfo()
  std::optional<ConfigAppliedInfo> overrideAgentConfigAppliedInfoForTesting_;

  using StateUpdateList = folly::IntrusiveList<
      TransceiverStateMachineUpdate,
      &TransceiverStateMachineUpdate::listHook_>;
  /*
   * A list of pending state updates to be applied.
   */
  folly::SpinLock pendingUpdatesLock_;
  StateUpdateList pendingUpdates_;

  /*
   * A thread for processing ModuleStateMachine updates.
   */
  std::unique_ptr<std::thread> updateThread_;
  std::unique_ptr<folly::EventBase> updateEventBase_;

  // TODO(joseph5wu) Will add heartbeat watchdog later

  // A global flag to indicate whether the service is exiting.
  // If it is, we should not accept any state update
  bool isExiting_{false};

  /*
   * A map to maintain all transceivers(present and absent) state machines.
   * As each platform has its own fixed supported module num
   * (`getNumQsfpModules()`), we'll only setup this map insude constructor,
   * and no other functions will erase any items from this map.
   */
  const TransceiverToStateMachineHelper stateMachines_;

  /*
   * A map to maintain all transceivers(present and absent) programmed SW port
   * to TransceiverPortInfo mapping.
   */
  const TransceiverToPortInfo tcvrToPortInfo_;

  /*
   * A ConfigAppliedInfo to keep track of the last wedge_agent config applied
   * info. refreshStateMachines() will routinely call wedge_agent thrift api to
   * getConfigAppliedInfo() thrift api, and then we can use that to tell
   * whether there's a config change. This will probably:
   * 1) introduce an updated iphy port profile change, like reloading config
   * with new speed;
   * 2) agent coldboot to reset iphy
   * And for the above cases, we need to issue a port re-programming from
   * iphy to xphy to tcvr.
   */
  ConfigAppliedInfo configAppliedInfo_;
};
} // namespace fboss
} // namespace facebook
