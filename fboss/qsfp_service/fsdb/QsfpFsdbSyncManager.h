/*
 *  Copyright (c) 2022-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/fsdb/client/FsdbSyncManager.h"
#include "fboss/lib/if/gen-cpp2/pim_state_types.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/port_state_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_service_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_state_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_stats_types.h"

namespace facebook {
namespace fboss {

class QsfpFsdbSyncManager {
 public:
  using TcvrStatsMap = std::map<int32_t, TcvrStats>;
  using TcvrStateMap = std::map<int32_t, TcvrState>;
  using PhyStatsMap = std::map<std::string, phy::PhyStats>;
  using PortStatsMap = std::map<std::string, HwPortStats>;
  using PimStatesMap = std::map<int, PimState>;

  QsfpFsdbSyncManager();

  static std::vector<std::string> getStatePath();
  static std::vector<std::string> getStatsPath();
  static std::vector<std::string> getConfigPath();

  void start();
  void stop();

  void updateConfig(cfg::QsfpServiceConfig newConfig);
  void updateTcvrStates(TcvrStateMap&& states);
  void updateTcvrStats(TcvrStatsMap&& stats);
  void updatePimState(int pimId, PimState&& PimState);
  void updatePhyState(
      std::string&& portName,
      std::optional<phy::PhyState>&& newState);
  void updatePhyStats(PhyStatsMap& stats);
  /// Update Phy stats one at a time
  ///
  /// Accumulates stats in pendingPhyStats_. Once all ports mentioned in Phy
  /// state are present, push out accumulated stats and resets it for the
  /// next cycle.
  void updatePhyStat(std::string&& portName, phy::PhyStats&& stat);

  // Accumuate all port stats in pendingPortStats_ (Phy Stats) and push them to
  // FSDB.
  void updatePortStats(PortStatsMap stats);
  void updatePortStat(std::string&& portName, HwPortStats&& stat);

  // Update the logical port state and send it to fsdb
  void updatePortState(std::string&& portName, portstate::PortState&& newState);

 private:
  std::unique_ptr<
      fsdb::FsdbSyncManager<state::QsfpServiceData, true /* EnablePatchAPIs */>>
      stateSyncer_;
  std::unique_ptr<fsdb::FsdbSyncManager<stats::QsfpStats>> statsSyncer_;
  // updatePhyStat, that reads and writes to pendingPhyStats_, is called per
  // port from every PIM evb. So calls to this function from ports on different
  // PIMs is racy, and hence needs to be synchronized
  folly::Synchronized<PhyStatsMap> pendingPhyStats_;
  folly::Synchronized<PortStatsMap> pendingPortStats_;
};

} // namespace fboss
} // namespace facebook
