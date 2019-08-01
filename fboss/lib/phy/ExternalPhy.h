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

#include <stdint.h>
#include <vector>

#include <folly/Range.h>

#include "fboss/lib/phy/gen-cpp2/phy_types.h"

namespace facebook {
namespace fboss {
namespace phy {

/*
 * API for programming external phy chips. Down the line all of these
 * structs should be represented in the SwitchState somehow.
 */

struct ExternalPhyLaneStats {
  float signalToNoiseRatio{0};
  uint64_t numAdapt{0};
  uint64_t numReadapt{0};
  uint64_t numLinkLost{0};
};

struct ExternalPhyPortSideStats {
  uint64_t fecUncorrectableErrors{0};
  // This is per lane
  std::unordered_map<uint16_t, ExternalPhyLaneStats> lanes;
};

struct ExternalPhyPortStats {
  ExternalPhyPortSideStats system;
  ExternalPhyPortSideStats line;
};

class ExternalPhy {
 public:
  virtual ~ExternalPhy() {}

  virtual PhyFwVersion fwVersion() = 0;

  // Program all settings in one go. This is a rather inflexible API
  // in that we expect to use all lanes in the external phy and that
  // all logical ports are the same speed on a given side of the phy.
  //
  // Down the line we can support mismatched speeds using more first
  // class integration with the switch state.
  virtual void program(PhySettings settings) = 0;
  virtual void programOnePort(PhyPortSettings settings) = 0;
  virtual PhySettings getSettings() = 0;

  virtual bool legalSettings(const PhySettings& settings) {
    // optionally overridable by subclasses
    return true;
  }
  // loopback
  virtual Loopback getLoopback(Side side) = 0;
  virtual void setLoopback(Side side, Loopback loopback) = 0;

  virtual ExternalPhyPortStats getPortStats(
      const PhyPortSettings& settings) = 0;

  virtual void reset() = 0;

  virtual void dump() {}
};

} // namespace phy
} // namespace fboss
} // namespace facebook
