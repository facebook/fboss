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
  bool verifySystemPorts();
  bool verifyRifs();
  bool verifyDsfSessions();

 private:
  void populateDsfNodes(
      const std::shared_ptr<MultiSwitchDsfNodeMap>& dsfNodeMap);
  std::set<std::string> getAllRdsws();

  std::set<std::string> getGlobalSystemPortsOfType(
      const std::string& rdsw,
      const std::set<RemoteSystemPortType>& types);
  bool verifySystemPortsForRdsw(const std::string& rdswToVerify);

  std::set<int> getGlobalRifsOfType(
      const std::string& rdsw,
      const std::set<RemoteInterfaceType>& types);
  bool verifyRifsForRdsw(const std::string& rdswToVerify);

  std::set<std::string> getRdswsWithEstablishedDsfSessions(
      const std::string& rdsw);

  std::map<int, std::vector<std::string>> clusterIdToRdsws_;
  std::map<int, std::vector<std::string>> clusterIdToFdsws_;
  std::set<std::string> sdsws_;

  std::map<SwitchID, std::string> switchIdToSwitchName_;
};

} // namespace facebook::fboss::utility
