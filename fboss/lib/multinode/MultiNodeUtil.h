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

#include <string>
#include "fboss/agent/state/DsfNodeMap.h"

namespace facebook::fboss::utility {

class MultiNodeUtil {
 public:
  explicit MultiNodeUtil(
      const std::shared_ptr<MultiSwitchDsfNodeMap>& dsfNodeMap);

  bool verifyFabricConnectivity() const;
  bool verifyFabricReachability() const;
  bool verifyPorts() const;
  bool verifySystemPorts() const;
  bool verifyRifs() const;
  bool verifyStaticNdpEntries() const;
  bool verifyDsfSessions() const;

  bool verifyGracefulFabricLinkDownUp() const;
  bool verifyGracefulDeviceDownUp() const;
  bool verifyUngracefulDeviceDownUp() const;
  bool verifyGracefulRestartTimeoutRecovery() const;

  bool verifyGracefulQsfpDownUp() const;
  bool verifyUngracefulQsfpDownUp() const;

  bool verifyGracefulFsdbDownUp() const;
  bool verifyUngracefulFsdbDownUp() const;

 private:
  enum class SwitchType : uint8_t {
    RDSW = 0,
    FDSW = 1,
    SDSW = 2,
  };

  std::string switchTypeToString(SwitchType switchType) const {
    switch (switchType) {
      case SwitchType::RDSW:
        return "RDSW";
      case SwitchType::FDSW:
        return "FDSW";
      case SwitchType::SDSW:
        return "SDSW";
    }
  }
  void populateDsfNodes(
      const std::shared_ptr<MultiSwitchDsfNodeMap>& dsfNodeMap);
  void populateAllRdsws();
  void populateAllFdsws();

  std::map<std::string, FabricEndpoint> getFabricEndpoints(
      const std::string& switchName) const;
  std::set<std::string> getConnectedFabricPorts(
      const std::string& switchName) const;
  bool verifyFabricConnectedSwitchesHelper(
      SwitchType switchType,
      const std::string& switchToVerify,
      const std::set<std::string>& expectedConnectedSwitches) const;
  bool verifyFabricConnectedSwitchesForRdsw(
      int clusterId,
      const std::string& rdswToVerify) const;
  bool verifyFabricConnectedSwitchesForAllRdsws() const;
  bool verifyFabricConnectedSwitchesForFdsw(
      int clusterId,
      const std::string& fdswToVerify) const;
  bool verifyFabricConnectedSwitchesForAllFdsws() const;
  bool verifyFabricConnectedSwitchesForSdsw(
      const std::string& sdswToVerify) const;
  bool verifyFabricConnectedSwitchesForAllSdsws() const;

  bool verifyFabricReachablityForRdsw(const std::string& rdswToVerify) const;

  bool verifyNoSessionsFlap(
      const std::string& rdswToVerify,
      const std::map<std::string, DsfSessionThrift>& baselinePeerToDsfSession,
      const std::optional<std::string>& rdswToExclude = std::nullopt) const;

  bool verifyNoSessionsEstablished(const std::string& rdswToVerify) const;
  bool verifyAllSessionsEstablished(const std::string& rdswToVerify) const;

  bool verifyGracefulFabricLinkDown(
      const std::string& rdswToVerify,
      const std::map<std::string, PortInfoThrift>&
          activeFabricPortNameToPortInfo) const;
  bool verifyGracefulFabricLinkUp(
      const std::string& rdswToVerify,
      const std::map<std::string, PortInfoThrift>&
          activeFabricPortNameToPortInfo) const;

  std::map<int32_t, facebook::fboss::PortInfoThrift> getPorts(
      const std::string& switchName) const;
  std::set<std::string> getActiveFabricPorts(
      const std::string& switchName) const;
  std::map<std::string, PortInfoThrift> getActiveFabricPortNameToPortInfo(
      const std::string& switchName) const;
  std::map<std::string, PortInfoThrift> getFabricPortNameToPortInfo(
      const std::string& switchName) const;

  bool verifyPortActiveStateForSwitch(
      SwitchType switchType,
      const std::string& switchName) const;
  bool verifyNoPortErrorsForSwitch(
      SwitchType switchType,
      const std::string& switchName) const;
  bool verifyPortsForSwitch(
      SwitchType switchType,
      const std::string& switchName) const;

  std::map<std::string, std::vector<SystemPortThrift>> getPeerToSystemPorts(
      const std::string& rdsw) const;
  std::set<std::string> getGlobalSystemPortsOfType(
      const std::string& rdsw,
      const std::set<RemoteSystemPortType>& types) const;
  bool verifySystemPortsForRdsw(const std::string& rdswToVerify) const;

  std::map<std::string, std::vector<InterfaceDetail>> getPeerToRifs(
      const std::string& rdsw) const;
  std::set<int> getGlobalRifsOfType(
      const std::string& rdsw,
      const std::set<RemoteInterfaceType>& types) const;
  bool verifyRifsForRdsw(const std::string& rdswToVerify) const;

  std::set<std::pair<std::string, std::string>> getNdpEntriesAndSwitchOfType(
      const std::string& rdsw,
      const std::set<std::string>& types) const;

  std::map<std::string, DsfSessionThrift> getPeerToDsfSession(
      const std::string& rdsw) const;
  std::set<std::string> getRdswsWithEstablishedDsfSessions(
      const std::string& rdsw) const;

  bool verifyDeviceDownUpForRemoteRdswsHelper(bool triggerGraceFulExit) const;

  bool verifyGracefulDeviceDownUpForRemoteRdsws() const;
  bool verifyGracefulDeviceDownUpForRemoteFdsws() const;
  bool verifyGracefulDeviceDownUpForRemoteSdsws() const;

  bool verifyUngracefulDeviceDownUpForRemoteRdsws() const;
  bool verifyUngracefulDeviceDownUpForRemoteFdsws() const;
  bool verifyUngracefulDeviceDownUpForRemoteSdsws() const;

  bool verifySwSwitchRunState(
      const std::string& rdswToVerify,
      const SwitchRunState& expectedSwitchRunState) const;
  bool verifyQsfpServiceRunState(
      const std::string& rdswToVerify,
      const QsfpServiceRunState& expectedQsfpRunState) const;
  bool verifyFsdbIsUp(const std::string& rdswToVerify) const;

  bool verifyStaleSystemPorts(
      const std::map<std::string, std::vector<SystemPortThrift>>&
          peerToSystemPorts,
      const std::set<std::string>& restartedRdsws) const;

  std::set<std::string> triggerGraceFulRestartTimeoutForRemoteRdsws() const;
  bool verifyStaleSystemPorts(
      const std::set<std::string>& restartedRdsws) const;
  bool verifyStaleRifs(const std::set<std::string>& restartedRdsws) const;
  bool verifyLiveSystemPorts() const;
  bool verifyLiveRifs() const;

  bool verifyUngracefulQsfpDownUpForRemoteRdsws() const;
  bool verifyUngracefulQsfpDownUpForRemoteFdsws() const;
  bool verifyUngracefulQsfpDownUpForRemoteSdsws() const;

  bool verifyFsdbDownUpForRemoteRdswsHelper(bool triggerGraceFulRestart) const;

  std::map<int, std::vector<std::string>> clusterIdToRdsws_;
  std::map<int, std::vector<std::string>> clusterIdToFdsws_;
  std::set<std::string> sdsws_;

  std::set<std::string> allRdsws_;
  std::set<std::string> allFdsws_;
  std::map<SwitchID, std::string> switchIdToSwitchName_;
};

} // namespace facebook::fboss::utility
