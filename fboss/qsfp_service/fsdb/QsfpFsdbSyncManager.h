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
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_service_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_state_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_stats_types.h"

namespace facebook {
namespace fboss {

class QsfpFsdbSyncManager {
 public:
  using TcvrStatsMap = std::map<int32_t, TcvrStats>;
  using PhyStatsMap = std::map<std::string, phy::PhyStats>;

  QsfpFsdbSyncManager();

  static std::vector<std::string> getStatePath();
  static std::vector<std::string> getStatsPath();
  static std::vector<std::string> getConfigPath();

  void start();
  void stop();

  void updateConfig(cfg::QsfpServiceConfig newConfig);
  void updateTcvrState(int32_t tcvrId, TcvrState&& newState);
  void updateTcvrStats(TcvrStatsMap&& stats);
  void updatePhyState(
      std::string&& portName,
      std::optional<phy::PhyState>&& newState);
  void updatePhyStats(PhyStatsMap&& stats);
  /// Update Phy stats one at a time
  ///
  /// Accumulates stats in pendingPhyStats_. Once all ports mentioned in Phy
  /// state are present, push out accumulated stats and resets it for the
  /// next cycle.
  void updatePhyStat(std::string&& portName, phy::PhyStats&& stat);

 private:
  std::unique_ptr<fsdb::FsdbSyncManager<state::QsfpServiceData>> stateSyncer_;
  std::unique_ptr<fsdb::FsdbSyncManager<stats::QsfpStats>> statsSyncer_;
  PhyStatsMap pendingPhyStats_;
};

} // namespace fboss
} // namespace facebook
