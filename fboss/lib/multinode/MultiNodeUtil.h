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

  bool verifyFabricConnectivity();
  bool verifyFabricReachability();
  bool verifyPorts();
  bool verifySystemPorts();
  bool verifyRifs();
  bool verifyStaticNdpEntries();
  bool verifyDsfSessions();

  bool verifyGracefulFabricLinkDownUp();

 private:
  enum class SwitchType : uint8_t {
    RDSW = 0,
    FDSW = 1,
    SDSW = 2,
  };

  std::string switchTypeToString(SwitchType switchType) {
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
      const std::string& switchName);
  std::set<std::string> getConnectedFabricPorts(const std::string& switchName);
  bool verifyFabricConnectedSwitchesHelper(
      SwitchType switchType,
      const std::string& switchToVerify,
      const std::set<std::string>& expectedConnectedSwitches);
  bool verifyFabricConnectedSwitchesForRdsw(
      int clusterId,
      const std::string& rdswToVerify);
  bool verifyFabricConnectedSwitchesForAllRdsws();
  bool verifyFabricConnectedSwitchesForFdsw(
      int clusterId,
      const std::string& fdswToVerify);
  bool verifyFabricConnectedSwitchesForAllFdsws();
  bool verifyFabricConnectedSwitchesForSdsw(const std::string& sdswToVerify);
  bool verifyFabricConnectedSwitchesForAllSdsws();

  bool verifyFabricReachablityForRdsw(const std::string& rdswToVerify);

  std::map<int32_t, facebook::fboss::PortInfoThrift> getPorts(
      const std::string& switchName);
  std::set<std::string> getActiveFabricPorts(const std::string& switchName);
  std::map<std::string, PortInfoThrift> getFabricPortNameToPortInfo(
      const std::string& switchName);

  bool verifyPortActiveStateForSwitch(
      SwitchType switchType,
      const std::string& switchName);
  bool verifyNoPortErrorsForSwitch(
      SwitchType switchType,
      const std::string& switchName);
  bool verifyPortsForSwitch(
      SwitchType switchType,
      const std::string& switchName);

  std::set<std::string> getGlobalSystemPortsOfType(
      const std::string& rdsw,
      const std::set<RemoteSystemPortType>& types);
  bool verifySystemPortsForRdsw(const std::string& rdswToVerify);

  std::set<int> getGlobalRifsOfType(
      const std::string& rdsw,
      const std::set<RemoteInterfaceType>& types);
  bool verifyRifsForRdsw(const std::string& rdswToVerify);

  std::set<std::pair<std::string, std::string>> getNdpEntriesAndSwitchOfType(
      const std::string& rdsw,
      const std::set<std::string>& types);

  std::set<std::string> getRdswsWithEstablishedDsfSessions(
      const std::string& rdsw);

  std::map<int, std::vector<std::string>> clusterIdToRdsws_;
  std::map<int, std::vector<std::string>> clusterIdToFdsws_;
  std::set<std::string> sdsws_;

  std::set<std::string> allRdsws_;
  std::set<std::string> allFdsws_;
  std::map<SwitchID, std::string> switchIdToSwitchName_;
};

} // namespace facebook::fboss::utility
