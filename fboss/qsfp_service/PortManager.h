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

#include <boost/bimap.hpp>
#include <string>
#include <unordered_map>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/types.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/qsfp_service/PortStateMachine.h"
#include "fboss/qsfp_service/TransceiverManager.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#define SW_PORT_LOG(level, logType, portName, portId)  \
  XLOG(level) << logType << " [portName: " << portName \
              << ", portId: " << portId << "]: "

#define PORT_SM_LOG(level, portName, portId) \
  SW_PORT_LOG(level, "[SM]", portName, portId)

namespace facebook::fboss {

class PortManager {
  using TcvrToPortMap = std::unordered_map<TransceiverID, std::vector<PortID>>;
  using PortToTcvrMap = std::unordered_map<PortID, std::vector<TransceiverID>>;
  using PortNameIdMap = boost::bimap<std::string, PortID>;

  using PortStateMachineController = StateMachineController<
      PortID,
      PortStateMachineEvent,
      PortStateMachineState,
      PortStateMachine>;
  using PortStateMachineUpdate = TypedStateMachineUpdate<PortStateMachineEvent>;
  using BlockingPortStateMachineUpdate =
      BlockingStateMachineUpdate<PortStateMachineEvent>;

 public:
  explicit PortManager(
      TransceiverManager* transceiverManager,
      PhyManager* phyManager,
      const std::shared_ptr<const PlatformMapping> platformMapping,
      const std::shared_ptr<std::unordered_map<TransceiverID, SlotThreadHelper>>
          threads);
  ~PortManager();
  void gracefulExit();

  void init();
  void syncPorts(
      std::map<int32_t, TransceiverInfo>& info,
      std::unique_ptr<std::map<int32_t, PortStatus>> ports);

  void publishPhyIOStats() const {
    if (!phyManager_) {
      return;
    }
    phyManager_->publishPhyIOStatsToFb303();
  }

  PhyManager* getPhyManager() {
    return phyManager_;
  }

  void programXphyPort(PortID portId, cfg::PortProfileID portProfileId);

  phy::PhyInfo getXphyInfo(PortID portId);

  void updateAllXphyPortsStats();

  std::vector<PortID> getMacsecCapablePorts() const;

  std::string listHwObjects(std::vector<HwObjectType>& hwObjects, bool cached)
      const;

  bool getSdkState(std::string filename) const;

  std::string getPortInfo(const std::string& portName);

  void setPortLoopbackState(
      const std::string& /* portName */,
      phy::PortComponent /* component */,
      bool /* setLoopback */);

  void setPortAdminState(
      const std::string& /* portName */,
      phy::PortComponent /* component */,
      bool /* setAdminUp */);

  void getSymbolErrorHistogram(
      CdbDatapathSymErrHistogram& symErr,
      const std::string& portName);

  std::string saiPhyRegisterAccess(
      const std::string& /* portName */,
      bool /* opRead */,
      int /* phyAddr */,
      int /* devId */,
      int /* regOffset */,
      int /* data */) {
    return "";
  }

  std::string saiPhySerdesRegisterAccess(
      const std::string& /* portName */,
      bool /* opRead */,
      int /* mdioAddr */,
      phy::Side /* side */,
      int /* serdesLane */,
      uint32_t /* regOffset */,
      uint32_t /* data */) {
    return "Function not implemented for this platform";
  }

  std::string phyConfigCheckHw(const std::string& /* portName */) {
    return "The function phyConfigCheckHw not implemented for this platform";
  }

  // Returns the interface names for a given transceiverId
  // Returns empty list when there is no corresponding name for a given
  // transceiver ID
  const std::set<std::string> getPortNames(TransceiverID tcvrId) const;

  // Returns the first interface name found for the given transceiverId
  // returns empty string when there is no name found
  const std::string getPortName(TransceiverID tcvrId) const;

  PortStateMachineState getPortState(PortID portId) const;

  TransceiverStateMachineState getTransceiverState(TransceiverID tcvrId) const;

  /*
   * This function is used to find the port (and port state machine) responsible
   * for driving programming functions that require all ports assigned to a
   * transceiver (e.g. programInternalPhyPorts).
   *
   * For 1:1 tcvr:port case, this will simply return the passed in portID.
   * For 1:X tcvr:port case, this will return the lowest portID assigned to the
   * transceiver.
   * For X:1 tcvr:port case, this will simply return the passed in portID.
   */
  PortID getLowestIndexedPortForTransceiverPortGroup(PortID portId) const;

  /* Gets lowest-indexed transceiverID associated with a given portID. This is
   * used for assigning port state machine functions to a thread for execution.
   *
   * For 1:1 tcvr:port case, this will return the assigned transceiverID.
   * For 1:X tcvr:port case, this will return the assigned transceiverID.
   * For X:1 tcvr:port case, this will return the lowest transceiverID assigned
   * to the port.
   */
  TransceiverID getLowestIndexedTransceiverForPort(PortID portId) const;

  bool isLowestIndexedPortForTransceiverPortGroup(PortID portId) const;

  // Gets all transceiverIDs for a given port. This will contain 2 transceivers
  // in the multi-tcvr - single-port use case, otherwise will contain 1.
  std::vector<TransceiverID> getTransceiverIdsForPort(PortID portId) const;

  bool hasPortFinishedIphyProgramming(PortID portId) const;

  void programInternalPhyPorts(TransceiverID id);

  void programExternalPhyPort(PortID portId, bool xphyNeedResetDataPath);

  phy::PhyInfo getPhyInfo(const std::string& portName);

  std::unordered_map<PortID, TransceiverManager::TransceiverPortInfo>
  getProgrammedIphyPortToPortInfo(TransceiverID id) const;

  // TEST ONLY
  void setOverrideAgentPortStatusForTesting(
      bool up,
      bool enabled,
      bool clearOnly = false);
  void setOverrideAgentConfigAppliedInfoForTesting(
      std::optional<ConfigAppliedInfo> configAppliedInfo);

  void getAllPortSupportedProfiles(
      std::map<std::string, std::vector<cfg::PortProfileID>>&
          supportedPortProfiles,
      bool checkOptics);

  // Function to convert port name string to software port id
  std::optional<PortID> getPortIDByPortName(const std::string& portName) const;

  // Function to convert port id to port name
  std::string getPortNameByPortId(PortID portId) const;

  std::vector<PortID> getAllPlatformPorts(TransceiverID tcvrID) const;

  void publishLinkSnapshots(const std::string& portName);

  void getInterfacePhyInfo(
      std::map<std::string, phy::PhyInfo>& phyInfos,
      const std::string& portName);

  void getAllInterfacePhyInfo(std::map<std::string, phy::PhyInfo>& phyInfos);

  void setPortPrbs(
      PortID portId,
      phy::PortComponent component,
      const phy::PortPrbsState& state);

  phy::PrbsStats getPortPrbsStats(PortID portId, phy::PortComponent component)
      const;

  void clearPortPrbsStats(PortID portId, phy::PortComponent component);

  void setInterfacePrbs(
      const std::string& portName,
      phy::PortComponent component,
      const prbs::InterfacePrbsState& state);

  phy::PrbsStats getInterfacePrbsStats(
      const std::string& portName,
      phy::PortComponent component) const;

  void getAllInterfacePrbsStats(
      std::map<std::string, phy::PrbsStats>& prbsStats,
      phy::PortComponent component) const;

  void clearInterfacePrbsStats(
      const std::string& portName,
      phy::PortComponent component);

  void bulkClearInterfacePrbsStats(
      std::unique_ptr<std::vector<std::string>> interfaces,
      phy::PortComponent component);

  void publishPhyStateToFsdb(
      std::string&& /* portName */,
      std::optional<phy::PhyState>&& /* newState */) const {}
  void publishPhyStatToFsdb(
      std::string&& /* portName */,
      phy::PhyStats&& /* stat */) const {}

  void publishPortStatToFsdb(
      std::string&& /* portName */,
      HwPortStats&& /* stat */) const {}

  void syncNpuPortStatusUpdate(
      std::map<int, facebook::fboss::NpuPortStatus>& portStatus);

  void triggerAgentConfigChangeEvent();

 protected:
  /*
   * function to initialize all the Phy in the system
   */
  bool initExternalPhyMap(bool forceWarmboot = false);

  void setPhyManager(std::unique_ptr<PhyManager> phyManager);
  void publishLinkSnapshots(PortID portId);

  // Restore phy state from the last cached warm boot qsfp_service state
  // Called this after initializing all the xphys during warm boot
  void restoreWarmBootPhyState();

  const std::shared_ptr<const PlatformMapping> platformMapping_;

  // Passed in through constructor.
  TransceiverManager* transceiverManager_;

  // For platforms that needs to program xphy (passed in through constructor).
  PhyManager* phyManager_;

  // Use the following bidirectional map to cache the static mapping so that we
  // don't have to search from PlatformMapping again and again
  PortNameIdMap portNameToPortID_;

 private:
  PortManager(PortManager const&) = delete;
  PortManager& operator=(PortManager const&) = delete;
  PortManager(PortManager&&) = delete;
  PortManager& operator=(PortManager&&) = delete;

  void updateTransceiverPortStatus() noexcept;

  void restoreAgentConfigAppliedInfo();

  // TEST ONLY
  // This private map is an override of agent getPortStatus()
  std::map<int32_t, NpuPortStatus> overrideAgentPortStatusForTesting_;
  // This ConfigAppliedInfo is an override of agent getConfigAppliedInfo()
  std::optional<ConfigAppliedInfo> overrideAgentConfigAppliedInfoForTesting_;

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

  void updateNpuPortStatusCache(
      std::map<int, facebook::fboss::NpuPortStatus>& portStatus);

  /*
   * This cache is updated from the FSDB subscription thread and read by the
   * main thread
   */
  folly::Synchronized<
      std::map<int /* agent logical port id */, facebook::fboss::NpuPortStatus>>
      npuPortStatusCache_;

  // A global flag to indicate whether the service is exiting.
  // If it is, we should not accept any state update
  std::atomic<bool> isExiting_{false};

  /*
   * A thread for processing PortStateMachine updates.
   */
  std::unique_ptr<std::thread> updateThread_;
  std::unique_ptr<folly::EventBase> updateEventBase_;
  std::shared_ptr<ThreadHeartbeat> updateThreadHeartbeat_;

  const std::shared_ptr<std::unordered_map<TransceiverID, SlotThreadHelper>>
      threads_;

  const TcvrToPortMap tcvrToPortMap_;
  const PortToTcvrMap portToTcvrMap_;

  using PortToStateMachineControllerMap =
      std::unordered_map<PortID, std::unique_ptr<PortStateMachineController>>;
  PortToStateMachineControllerMap setupPortToStateMachineControllerMap();
  const PortToStateMachineControllerMap stateMachineControllers_;
};

} // namespace facebook::fboss
