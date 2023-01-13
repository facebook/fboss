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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/platform_config_types.h"
#include "fboss/agent/types.h"
#include "fboss/lib/AlertLogger.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "folly/dynamic.h"

namespace facebook::fboss::phy {

/*
 * API for programming external phy chips. Down the line all of these
 * structs should be represented in the SwitchState somehow.
 */

struct ExternalPhyLaneStats {
  bool signalDetect{false};
  bool cdrLock{false};
  float signalToNoiseRatio{0};
  uint64_t numAdapt{0};
  uint64_t numReadapt{0};
  uint64_t numLinkLost{0};
  std::optional<int64_t> prbsErrorCounts;
  std::optional<int32_t> prbsLock;
  std::optional<int32_t> prbsLockLoss;
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

  static ExternalPhyPortStats fromPhyInfo(const PhyInfo& phyInfo);
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
  folly::dynamic toDynamic() const;
};

struct PhySideConfig {
  std::map<LaneID, LaneConfig> lanes;

  bool operator==(const PhySideConfig& rhs) const;
  folly::dynamic toDynamic() const;
  std::vector<PinConfig> getPinConfigs() const;
};

struct ExternalPhyConfig {
  PhySideConfig system;
  PhySideConfig line;

  bool operator==(const ExternalPhyConfig& rhs) const;
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
  folly::dynamic toDynamic() const;
};

struct PhyPortConfig {
  ExternalPhyConfig config;
  ExternalPhyProfileConfig profile;

  bool operator==(const PhyPortConfig& rhs) const;
  bool operator!=(const PhyPortConfig& rhs) const;
  folly::dynamic toDynamic() const;

  int getLaneSpeedInMb(Side side) const;
};

// Structure for keeping port phy information for a port
struct PhyIDInfo {
  PimID pimID;
  MdioControllerID controllerID;
  // TODO(ccpowers): this is currently being used as a phy ID rather than an
  // address, need to correct this.
  PhyAddr phyAddr;

  bool operator==(const PhyIDInfo& other) const {
    return std::tie(pimID, controllerID, phyAddr) ==
        std::tie(other.pimID, other.controllerID, other.phyAddr);
  }

  bool operator!=(const PhyIDInfo& other) const {
    return !(*this == other);
  }
  std::string str() const;
};

std::string getLaneListStr(const std::vector<facebook::fboss::LaneID>& lanes);

class ExternalPhy {
 public:
  enum class Feature {
    LOOPBACK,
    PRBS,
    PRBS_STATS,
    MACSEC,
    PORT_STATS,
    PORT_INFO,
  };
  static std::string featureName(Feature feature);

  virtual ~ExternalPhy() {}

  virtual PhyFwVersion fwVersion() = 0;

  void logPortProgrammed(
      int32_t phyId,
      std::string sysLanes,
      std::string lineLanes,
      std::string speed) {
    XLOG(INFO) << PlatformAlert() << "Programmed phy id=" << phyId
               << " system lanes=" << sysLanes << " line lanes=" << lineLanes
               << " speed=" << speed;
  }

  virtual void programOnePort(
      PhyPortConfig config,
      bool /* needResetDataPath */) = 0;

  virtual bool isSupported(Feature feature) const = 0;

  virtual bool legalOnePortConfig(const PhyPortConfig& /* config */) {
    // optionally overridable by subclasses
    return true;
  }

  virtual PhyPortConfig getConfigOnePort(
      const std::vector<LaneID>& sysLanes,
      const std::vector<LaneID>& lineLanes) = 0;
  // loopback
  virtual Loopback getLoopback(Side side) = 0;
  virtual void setLoopback(Side side, Loopback loopback) = 0;

  virtual void setPortPrbs(
      Side side,
      const std::vector<LaneID>& lanes,
      const PortPrbsState& prbs) = 0;

  virtual PortPrbsState getPortPrbs(
      Side side,
      const std::vector<LaneID>& lanes) = 0;

  virtual std::vector<ExternalPhyLaneDiagInfo> getOnePortDiagInfo(
      uint32_t sysLanes,
      uint32_t lineLanes) {
    return std::vector<ExternalPhyLaneDiagInfo>();
  };

  virtual ExternalPhyPortStats getPortStats(
      const std::vector<LaneID>& sysLanes,
      const std::vector<LaneID>& lineLanes) = 0;

  virtual PhyInfo getPortInfo(
      const std::vector<LaneID>& /* sysLanes */,
      const std::vector<LaneID>& /* lineLanes */,
      PhyInfo& /* lastPhyInfo */) {
    throw facebook::fboss::FbossError(
        "Port info not supported on this platform");
  }

  virtual ExternalPhyPortStats getPortPrbsStats(
      const std::vector<LaneID>& sysLanes,
      const std::vector<LaneID>& lineLanes) = 0;

  virtual void reset() = 0;

  virtual void dump() {}

  static bool compareConfigs(
      const PhyPortConfig& lhs,
      const PhyPortConfig& rhs) {
    return lhs == rhs;
  }
};

} // namespace facebook::fboss::phy
