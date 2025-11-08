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

#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/agent_multinode_tests/TopologyInfo.h"

namespace facebook::fboss::utility {

class DsfTopologyInfo : public TopologyInfo {
 public:
  explicit DsfTopologyInfo(const std::shared_ptr<SwitchState>& switchState)
      : TopologyInfo(switchState) {
    populateDsfNodeInfo(switchState->getDsfNodes());
    populateAllRdsws();
    populateAllFdsws();
    populateAllSwitches();
  }

  const std::map<int, std::vector<std::string>>& getClusterIdToRdsws()
      const override {
    return clusterIdToRdsws_;
  }

  const std::map<int, std::vector<std::string>>& getClusterIdToFdsws()
      const override {
    return clusterIdToFdsws_;
  }

  const std::set<std::string>& getRdsws() const override {
    return allRdsws_;
  }

  const std::set<std::string>& getFdsws() const override {
    return allFdsws_;
  }

  const std::set<std::string>& getSdsws() const override {
    return sdsws_;
  }

  const std::map<SwitchID, std::string>& getSwitchIdToSwitchName()
      const override {
    return switchIdToSwitchName_;
  }

  const std::map<std::string, std::set<SwitchID>>& getSwitchNameToSwitchIds()
      const override {
    return switchNameToSwitchIds_;
  }

  const std::map<std::string, cfg::AsicType>& getSwitchNameToAsicType()
      const override {
    return switchNameToAsicType_;
  }

  const std::map<std::string, cfg::SystemPortRanges>
  getSwitchNameToSystemPortRanges() const override {
    return switchNameToSystemPortRanges_;
  }

 private:
  void populateDsfNodeInfo(
      const std::shared_ptr<MultiSwitchDsfNodeMap>& dsfNodeMap);
  void populateAllRdsws();
  void populateAllFdsws();
  void populateAllSwitches() override;

  std::map<int, std::vector<std::string>> clusterIdToRdsws_;
  std::map<int, std::vector<std::string>> clusterIdToFdsws_;
  std::set<std::string> sdsws_;

  std::map<SwitchID, std::string> switchIdToSwitchName_;
  std::map<std::string, std::set<SwitchID>> switchNameToSwitchIds_;
  std::map<std::string, cfg::AsicType> switchNameToAsicType_;
  std::map<std::string, cfg::SystemPortRanges> switchNameToSystemPortRanges_;

  std::set<std::string> allRdsws_;
  std::set<std::string> allFdsws_;
};

} // namespace facebook::fboss::utility
