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

#include "fboss/agent/gen-cpp2/platform_config_types.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "folly/dynamic.h"

namespace facebook::fboss::phy {

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
  uint64_t fecCorrectableErrors{0};
  // This is per lane
  std::unordered_map<uint16_t, ExternalPhyLaneStats> lanes;
};

struct ExternalPhyPortStats {
  ExternalPhyPortSideStats system;
  ExternalPhyPortSideStats line;
};

struct ExternalPhyLaneDiagInfo {
  Side side;
  int16_t lane{-1};
  int16_t signal_detect{-1};
  int16_t cdr_lock{-1};
  int16_t osr_mode{-1};
  int16_t vga{-1};
  int16_t rx_ppm{0};
  int16_t tx_ppm{0};
  int16_t txfir_pre2{0};
  int16_t txfir_pre{0};
  int16_t txfir_main{0};
  int16_t txfir_post1{0};
  int16_t txfir_post2{0};
  int16_t txfir_post3{0};
  int16_t modulation{-1};
  int16_t peaking_filter{0};
  float snr{0};
};

struct LaneConfig {
  std::optional<PolaritySwap> polaritySwap;
  std::optional<TxSettings> tx;

  bool operator==(const LaneConfig& rhs) const;
  LaneSettings toLaneSettings() const;
  static LaneConfig fromLaneSettings(const LaneSettings& settings);
  std::string str();
  folly::dynamic toDynamic() const;
};

struct PhySideConfig {
  std::map<int32_t, LaneConfig> lanes;

  bool operator==(const PhySideConfig& rhs) const;
  std::string str();
  folly::dynamic toDynamic() const;
  static PhySideConfig fromPhyPortSideSettings(const PhyPortSideSettings& rhs);
};

struct ExternalPhyConfig {
  PhySideConfig system;
  PhySideConfig line;

  bool operator==(const ExternalPhyConfig& rhs) const;
  std::string str();
  folly::dynamic toDynamic() const;
  static ExternalPhyConfig fromConfigeratorTypes(
      PortPinConfig portPinConfig,
      const std::map<int32_t, PolaritySwap>& linePolaritySwapMap);
};

// Same thing as PortProfileConfig, but without xphyLine being optional
// So we only need to null-check it once
struct ExternalPhyProfileConfig {
  cfg::PortSpeed speed;
  ProfileSideConfig system;
  ProfileSideConfig line;

  bool operator==(const ExternalPhyProfileConfig& rhs) const;
  static ExternalPhyProfileConfig fromPortProfileConfig(
      const PortProfileConfig& portCfg);
  std::string str();
  folly::dynamic toDynamic() const;
};

struct PhyPortConfig {
  ExternalPhyConfig config;
  ExternalPhyProfileConfig profile;

  bool operator==(const PhyPortConfig& rhs) const;
  bool operator!=(const PhyPortConfig& rhs) const;
  PhyPortSettings toPhyPortSettings(int16_t phyID) const;
  static PhyPortConfig fromPhyPortSettings(const PhyPortSettings& settings);
  std::string str();
  folly::dynamic toDynamic() const;
};

class ExternalPhy {
 public:
  virtual ~ExternalPhy() {}

  virtual PhyFwVersion fwVersion() = 0;

  /* TODO (ccpowers): remove all of these old functions using
    - PhyPortSettings
    - PhySettings
    once the new configuration format is fully supported */

  // Program all settings in one go. This is a rather inflexible API
  // in that we expect to use all lanes in the external phy and that
  // all logical ports are the same speed on a given side of the phy.
  //
  // Down the line we can support mismatched speeds using more first
  // class integration with the switch state.
  virtual void program(PhySettings settings) = 0;
  virtual void programOnePort(PhyPortSettings settings) = 0;
  virtual void programOnePort(PhyPortConfig config) = 0;
  virtual PhySettings getSettings() = 0;

  virtual bool legalSettings(const PhySettings& settings) {
    // optionally overridable by subclasses
    return true;
  }
  virtual bool legalOnePortConfig(const PhyPortConfig& /* config */) {
    // optionally overridable by subclasses
    return true;
  }
  virtual bool legalOnePortSettings(const PhyPortSettings& /* config */) {
    // optionally overridable by subclasses
    return true;
  }
  // loopback
  virtual Loopback getLoopback(Side side) = 0;
  virtual void setLoopback(Side side, Loopback loopback) = 0;

  virtual ExternalPhyPortStats getPortStats(
      const PhyPortSettings& settings) = 0;
  virtual std::vector<ExternalPhyLaneDiagInfo> getOnePortDiagInfo(
      uint32_t sysLanes,
      uint32_t lineLanes) {
    return std::vector<ExternalPhyLaneDiagInfo>();
  };

  virtual ExternalPhyPortStats getPortStats(const PhyPortConfig& config) = 0;

  virtual void reset() = 0;

  virtual void dump() {}

  static bool compareConfigs(
      const PhyPortConfig& lhs,
      const PhyPortConfig& rhs) {
    return lhs == rhs;
  }
};

} // namespace facebook::fboss::phy
