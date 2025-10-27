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

 private:
  TopologyType topologyType_;
};

} // namespace facebook::fboss::utility
