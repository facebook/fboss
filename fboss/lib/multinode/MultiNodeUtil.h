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

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/DsfNodeMap.h"

#include <folly/MacAddress.h>
#include <string>

namespace facebook::fboss::utility {

class MultiNodeUtil {
 public:
  explicit MultiNodeUtil(
      SwSwitch* sw,
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

  bool verifyNeighborAddRemove() const;

  bool verifyTrafficSpray() const;
  bool verifyNoTrafficDropOnProcessRestarts() const;
  bool verifyNoTrafficDropOnDrainUndrain() const;

  bool verifySelfHealingECMPLag() const;

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

  void logNdpEntry(
      const std::string& rdsw,
      const facebook::fboss::NdpEntryThrift& ndpEntry) const {
    auto ip = folly::IPAddress::fromBinary(
        folly::ByteRange(
            folly::StringPiece(ndpEntry.ip().value().addr().value())));

    XLOG(DBG2) << "From " << rdsw << " ip: " << ip.str()
               << " state: " << ndpEntry.state().value()
               << " switchId: " << ndpEntry.switchId().value_or(-1);
  }

  void logRdswToNdpEntries(
      const std::map<std::string, std::vector<NdpEntryThrift>>&
          rdswToNdpEntries) const {
    for (const auto& [rdsw, ndpEntries] : rdswToNdpEntries) {
      for (const auto& ndpEntry : ndpEntries) {
        auto ndpEntryIp = folly::IPAddress::fromBinary(
            folly::ByteRange(
                folly::StringPiece(ndpEntry.ip().value().addr().value())));
        XLOG(DBG2) << "RDSW:: " << rdsw
                   << " NDP Entry to verify:: port: " << ndpEntry.port().value()
                   << " interfaceID: " << ndpEntry.interfaceID().value()
                   << " IP: " << ndpEntryIp;
      }
    }
  }

  void logRif(
      const std::string& rdsw,
      const facebook::fboss::InterfaceDetail& rif) {
    XLOG(DBG2)
        << "From " << rdsw << " interfaceName: " << rif.interfaceName().value()
        << " interfaceId: " << rif.interfaceId().value() << " remoteIntfType: "
        << apache::thrift::util::enumNameSafe(rif.remoteIntfType().value_or(-1))
        << " remoteIntfLivenessStatus: "
        << folly::to<std::string>(rif.remoteIntfLivenessStatus().value_or(-1))
        << " scope: "
        << apache::thrift::util::enumNameSafe(rif.scope().value());
  }

  void logSystemPort(
      const std::string& rdsw,
      const facebook::fboss::SystemPortThrift& systemPort) {
    XLOG(DBG2) << "From " << rdsw << " portId: " << systemPort.portId().value()
               << " switchId: " << systemPort.switchId().value()
               << " portName: " << systemPort.portName().value()
               << " remoteSystemPortType: "
               << apache::thrift::util::enumNameSafe(
                      systemPort.remoteSystemPortType().value_or(-1))
               << " remoteSystemPortLivenessStatus: "
               << apache::thrift::util::enumNameSafe(
                      systemPort.remoteSystemPortLivenessStatus().value_or(-1))
               << " scope: "
               << apache::thrift::util::enumNameSafe(
                      systemPort.scope().value());
  }

  void populateDsfNodes(
      const std::shared_ptr<MultiSwitchDsfNodeMap>& dsfNodeMap);
  void populateAllRdsws();
  void populateAllFdsws();
  void populateAllSwitches();

  std::map<std::string, FabricEndpoint> getConnectedFabricPortToFabricEndpoint(
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

  std::set<std::string> getActiveFabricPorts(
      const std::string& switchName) const;
  std::map<std::string, PortInfoThrift> getActiveFabricPortNameToPortInfo(
      const std::string& switchName) const;
  std::map<std::string, PortInfoThrift> getFabricPortNameToPortInfo(
      const std::string& switchName) const;
  std::map<std::string, PortInfoThrift> getUpEthernetPortNameToPortInfo(
      const std::string& switchName) const;

  bool verifyPortActiveStateForSwitch(
      SwitchType switchType,
      const std::string& switchName) const;
  bool verifyNoPortErrorsForSwitch(
      SwitchType switchType,
      const std::string& switchName) const;
  bool verifyPortCableLength(
      SwitchType switchType,
      const std::string& switchName) const;
  bool verifyPortsForSwitch(
      SwitchType switchType,
      const std::string& switchName) const;

  std::map<std::string, std::vector<SystemPortThrift>> getRdswToSystemPorts()
      const;
  std::set<std::string> getGlobalSystemPortsOfType(
      const std::string& rdsw,
      const std::set<RemoteSystemPortType>& types) const;
  bool verifySystemPortsForRdsw(const std::string& rdswToVerify) const;

  std::map<std::string, std::vector<InterfaceDetail>> getRdswToRifs() const;
  std::set<int> getGlobalRifsOfType(
      const std::string& rdsw,
      const std::set<RemoteInterfaceType>& types) const;
  bool verifyRifsForRdsw(const std::string& rdswToVerify) const;

  std::vector<NdpEntryThrift> getNdpEntriesOfType(
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

  struct NeighborInfo {
    int32_t portID = 0;
    int32_t intfID = 0;
    folly::IPAddress ip = folly::IPAddress("::");
    folly::MacAddress mac = folly::MacAddress("00:00:00:00:00:00");

    std::string str() const {
      return folly::to<std::string>(
          "portID: ",
          portID,
          ", intfID: ",
          intfID,
          ", ip: ",
          ip.str(),
          ", mac: ",
          mac.toString());
    }
  };
  std::vector<NeighborInfo> computeNeighborsForRdsw(
      const std::string& rdsw,
      const int& numNeighbors) const;

  // if allNeighborsMustBePresent is true, then all neighbors must be present
  // for every rdsw in rdswToNdpEntries.
  // if allNeighborsMustBePresent is false, then all neighbors must be absent
  // for every rdsw in rdswToNdpEntries.
  bool verifyNeighborHelper(
      const std::vector<MultiNodeUtil::NeighborInfo>& neighbors,
      const std::map<std::string, std::vector<NdpEntryThrift>>&
          rdswToNdpEntries,
      bool allNeighborsMustBePresent) const;
  bool verifyNeighborsPresent(
      const std::string& rdswToVerify,
      const std::vector<MultiNodeUtil::NeighborInfo>& neighbors) const;
  bool verifyNeighborLocalPresent(
      const std::string& rdsw,
      const std::vector<MultiNodeUtil::NeighborInfo>& neighbors) const;
  bool verifyNeighborsAbsent(
      const std::vector<MultiNodeUtil::NeighborInfo>& neighbors,
      const std::optional<std::string>& rdswToExclude = std::nullopt) const;
  bool verifyNeighborLocalPresentRemoteAbsent(
      const std::vector<MultiNodeUtil::NeighborInfo>& neighbors,
      const std::string& rdsw) const;

  std::pair<folly::IPAddressV6, int16_t> kGetRoutePrefixAndPrefixLength()
      const {
    return std::make_pair(folly::IPAddressV6("2001:0db8:85a3::"), 64);
  }

  bool verifyRoutePresent(
      const std::string& rdsw,
      const folly::IPAddress& destPrefix,
      const int16_t prefixLength) const;

  bool verifyLineRate(
      const std::string& rdsw,
      const MultiNodeUtil::NeighborInfo& neighborInfo) const;
  bool verifyFabricSpray(const std::string& rdsw) const;

  std::map<std::string, NeighborInfo>
  configureNeighborsAndRoutesForTrafficLoop() const;
  void createTrafficLoop(const NeighborInfo& neighborInfo) const;
  bool setupTrafficLoop() const;

  bool verifyNoReassemblyErrorsForAllSwitches() const;

  struct Scenario {
    std::string name;
    std::function<bool()> setup;
  };
  // Return true only if all scenarios are successful
  bool runScenariosAndVerifyNoDrops(
      const std::vector<Scenario>& scenarios) const;

  bool drainUndrainActiveFabricLinkForSwitch(
      const std::string& switchName) const;

  // Returns sample set of Fabric switches to test.
  // One FDSW from each cluster + one SDSW.
  std::set<std::string> getOneFabricSwitchForEachCluster() const;

  std::map<int, std::vector<std::string>> clusterIdToRdsws_;
  std::map<int, std::vector<std::string>> clusterIdToFdsws_;
  std::set<std::string> sdsws_;

  std::set<std::string> allRdsws_;
  std::set<std::string> allFdsws_;
  std::map<SwitchID, std::string> switchIdToSwitchName_;
  std::map<std::string, std::set<SwitchID>> switchNameToSwitchIds_;
  std::map<std::string, cfg::AsicType> switchNameToAsicType_;
  std::set<std::string> allSwitches_;

  SwSwitch* sw_{nullptr};
};

} // namespace facebook::fboss::utility
