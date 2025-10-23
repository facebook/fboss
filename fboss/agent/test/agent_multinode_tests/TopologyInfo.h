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
  explicit TopologyInfo(const std::shared_ptr<SwitchState>& switchState);

  bool isTestDriver(const SwSwitch& sw) const;

  enum class TopologyType : uint8_t {
    DSF = 0,
  };
  TopologyType getTopologyType() const {
    return topologyType_;
  }

 private:
  void populateTopologyType(const std::shared_ptr<SwitchState>& switchState);

  TopologyType topologyType_;
};

} // namespace facebook::fboss::utility
