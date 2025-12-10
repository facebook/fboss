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
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss::utility {

class TopologyInfo {
 public:
  static std::unique_ptr<TopologyInfo> makeTopologyInfo(
      const std::shared_ptr<SwitchState>& switchState);

  explicit TopologyInfo(const std::shared_ptr<SwitchState>& switchState);
  virtual ~TopologyInfo();

  bool isTestDriver(const SwSwitch& sw) const;

  enum class TopologyType : uint8_t {
    DSF = 0,
  };
  TopologyType getTopologyType() const {
    return topologyType_;
  }

  std::string getMyHostname() const {
    return myHostname_;
  }

  virtual const std::map<int, std::vector<std::string>>& getClusterIdToRdsws()
      const = 0;
  virtual const std::map<int, std::vector<std::string>>& getClusterIdToFdsws()
      const = 0;
  virtual const std::set<std::string>& getSdsws() const = 0;
  virtual const std::set<std::string>& getRdsws() const = 0;
  virtual const std::set<std::string>& getFdsws() const = 0;
  virtual const std::map<SwitchID, std::string>& getSwitchIdToSwitchName()
      const = 0;
  virtual const std::map<std::string, std::set<SwitchID>>&
  getSwitchNameToSwitchIds() const = 0;
  virtual const std::map<std::string, cfg::AsicType>& getSwitchNameToAsicType()
      const = 0;
  virtual const std::map<std::string, cfg::SystemPortRanges>
  getSwitchNameToSystemPortRanges() const = 0;

  // Each TopologyInfo subclass e.g. DsfTopologyInfo populates switches in that
  // topology
  virtual void populateAllSwitches() = 0;
  const std::set<std::string>& getAllSwitches() const {
    return allSwitches_;
  }

 protected:
  std::set<std::string> allSwitches_;

 private:
  TopologyType topologyType_;

  // The Test binary only runs on the TestDriver.
  // Thus, My hostname is also the TestDriver name.
  std::string myHostname_;
};

} // namespace facebook::fboss::utility
