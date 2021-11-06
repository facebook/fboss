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
#include "fboss/agent/platforms/common/PlatformMode.h"
#include "fboss/agent/types.h"
#include "fboss/lib/i2c/gen-cpp2/i2c_controller_stats_types.h"
#include "fboss/lib/phy/PhyManager.h"
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
   * Virtual functions to get the cached transceiver signal flags and media lane
   * signals and clear the cached data. This is introduced mainly due to the
   * mismatch of ODS reporting frequency and the interval of us reading
   * transceiver data. Some of the clear on read information may be lost in this
   * process and not being captured in the ODS time series. This would bring
   * difficulty in root cause link issues. Thus here we provide a way of read
   * and clear the data for the purpose of ODS data reporting.
   */
  virtual void getAndClearTransceiversSignalFlags(
      std::map<int32_t, SignalFlags>& signalFlagsMap,
      std::unique_ptr<std::vector<int32_t>> ids) = 0;
  virtual void getAndClearTransceiversMediaSignals(
      std::map<int32_t, std::map<int, MediaLaneSignals>>& mediaSignalsMap,
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
      int32_t portId,
      cfg::PortProfileID portProfileId) = 0;

  virtual void updateAllXphyPortsStats() = 0;

  const PortGroups& getModuleToPortMap() const {
    return portGroupMap_;
  }

  const PortNameMap& getPortNameToModuleMap() const;

  const QsfpConfig* getQsfpConfig() const {
    return qsfpConfig_.get();
  };
  virtual std::vector<PortID> getMacsecCapablePorts() const = 0;

  // Returns the interface names for a given transceiverId
  // Returns empty list when there is no corresponding name for a given
  // transceiver ID
  const std::set<std::string> getPortNames(TransceiverID tcvrId) const;

  // Returns the first interface name found for the given transceiverId
  // returns empty string when there is no name found
  const std::string getPortName(TransceiverID tcvrId) const;

  void updateState(TransceiverID id, TransceiverStateMachineEvent event);
  void updateStateBlocking(
      TransceiverID id,
      TransceiverStateMachineEvent event);

  TransceiverStateMachineState getCurrentState(TransceiverID id) const;

  // ========== Public functions for TransceiverStateMachine ==========
  void programInternalPhyPorts(TransceiverID id);

  void programExternalPhyPorts(TransceiverID id);

  void programTransceiver(TransceiverID id);

  // ========== Public functions for TransceiverStateMachine ==========
  // A struct to keep track of the software port profile and status
  struct TransceiverPortInfo {
    cfg::PortProfileID profile;
    std::optional<PortStatus> status;
  };
  std::unordered_map<PortID, TransceiverPortInfo>
  getProgrammedIphyPortToPortInfo(TransceiverID id) const;

  std::unordered_map<PortID, cfg::PortProfileID>
  getOverrideProgrammedIphyPortAndProfileForTest(TransceiverID id) const;

  // If the transceiver doesn't exit, it will still return a TransceiverInfo
  // with present filed is false.
  TransceiverInfo getTransceiverInfo(TransceiverID id);

 protected:
  virtual void loadConfig() = 0;

  // Check whether iphy/xphy/transceiver programmed is done. If not, then
  // trigger the corresponding program event to program the component.
  void triggerProgrammingEvents();

  void setPhyManager(std::unique_ptr<PhyManager> phyManager) {
    phyManager_ = std::move(phyManager);
  }

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

 private:
  // Forbidden copy constructor and assignment operator
  TransceiverManager(TransceiverManager const&) = delete;
  TransceiverManager& operator=(TransceiverManager const&) = delete;

  using TransceiverToStateMachine = std::unordered_map<
      TransceiverID,
      std::unique_ptr<
          folly::Synchronized<state_machine<TransceiverStateMachine>>>>;
  TransceiverToStateMachine setupTransceiverToStateMachine();

  using TransceiverToPortInfo = std::unordered_map<
      TransceiverID,
      std::unique_ptr<folly::Synchronized<
          std::unordered_map<PortID, TransceiverPortInfo>>>>;
  TransceiverToPortInfo setupTransceiverToPortInfo();

  void startThreads();
  void stopThreads();
  void threadLoop(folly::StringPiece name, folly::EventBase* eventBase);

  void updateState(std::unique_ptr<TransceiverStateMachineUpdate> update);

  static void handlePendingUpdatesHelper(TransceiverManager* mgr);
  void handlePendingUpdates();

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
  const TransceiverToStateMachine stateMachines_;

  /*
   * A map to maintain all transceivers(present and absent) programmed SW port
   * to TransceiverPortInfo mapping.
   */
  const TransceiverToPortInfo tcvrToPortInfo_;
};
} // namespace fboss
} // namespace facebook
