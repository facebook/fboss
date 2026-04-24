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

#include "fboss/agent/hw/sai/api/DebugCounterApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"

#include <memory>

namespace facebook::fboss {

class SaiManagerTable;
class StateDelta;
class SaiStore;
class SaiPlatform;

using SaiInPortDebugCounter = SaiObject<SaiInPortDebugCounterTraits>;
using SaiOutPortDebugCounter = SaiObject<SaiOutPortDebugCounterTraits>;
using SaiInSwitchDebugCounter = SaiObject<SaiInSwitchDebugCounterTraits>;

class SaiDebugCounterManager {
 public:
  SaiDebugCounterManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      SaiPlatform* platform)
      : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {}

  void setupDebugCounters();
  sai_stat_id_t getPortL3BlackHoleCounterStatId() const {
    return portL3BlackHoleCounterStatId_;
  }
  sai_stat_id_t getMPLSLookupFailedCounterStatId() const {
    return mplsLookupFailCounterStatId_;
  }
  sai_stat_id_t getAclDropCounterStatId() const {
    return aclDropCounterStatId_;
  }
  sai_stat_id_t getTrapDropCounterStatId() const {
    return trapDropCounterStatId_;
  }
  sai_stat_id_t getEgressForwardingDropStatId() const {
    return egressForwardingDropCounterStatId_;
  }
  sai_stat_id_t getL2SwitchDropCounterStatId() const {
    return l2SwitchDropCounterStatId_;
  }
  sai_stat_id_t getL3SwitchDropCounterStatId() const {
    return l3SwitchDropCounterStatId_;
  }
  sai_stat_id_t getTunnelSwitchDropCounterStatId() const {
    return tunnelSwitchDropCounterStatId_;
  }
  sai_stat_id_t getSrv6MySidSwitchDropCounterStatId() const {
    return srv6MySidSwitchDropCounterStatId_;
  }
#if SAI_API_VERSION >= SAI_VERSION(1, 9, 0)
  sai_stat_id_t getSrv6MySidDropCounterStatId() const {
    return srv6MySidDropCounterStatId_;
  }
#endif
  std::set<sai_stat_id_t> getConfiguredDebugStatIds() const;
  std::set<sai_stat_id_t> getConfiguredSwitchDebugStatIds() const;

 private:
  static sai_stat_id_t kInvalidStatId() {
    return std::numeric_limits<sai_stat_id_t>::max();
  }
  void setupPortL3BlackHoleCounter();
  void setupMPLSLookupFailedCounter();
  void setupAclDropCounter();
  void setupTrapDropCounter();
  void setupEgressForwardingDropCounter();
  void setupL2SwitchDropCounter();
  void setupL3SwitchDropCounter();
  void setupTunnelSwitchDropCounter();
  void setupSrv6MySidSwitchDropCounter();
#if SAI_API_VERSION >= SAI_VERSION(1, 9, 0)
  void setupSrv6MySidDropCounter();
#endif
  std::shared_ptr<SaiInPortDebugCounter> portL3BlackHoleCounter_;
  std::shared_ptr<SaiInPortDebugCounter> mplsLookupFailCounter_;
  std::shared_ptr<SaiInPortDebugCounter> aclDropCounter_;
  std::shared_ptr<SaiInPortDebugCounter> trapDropCounter_;
  std::shared_ptr<SaiOutPortDebugCounter> egressForwardingDropCounter_;
  std::shared_ptr<SaiInSwitchDebugCounter> l2SwitchDropCounter_;
  std::shared_ptr<SaiInSwitchDebugCounter> l3SwitchDropCounter_;
  std::shared_ptr<SaiInSwitchDebugCounter> tunnelSwitchDropCounter_;
  std::shared_ptr<SaiInSwitchDebugCounter> srv6MySidSwitchDropCounter_;
#if SAI_API_VERSION >= SAI_VERSION(1, 9, 0)
  std::shared_ptr<SaiInPortDebugCounter> srv6MySidDropCounter_;
  sai_stat_id_t srv6MySidDropCounterStatId_{kInvalidStatId()};
#endif
  sai_stat_id_t portL3BlackHoleCounterStatId_{kInvalidStatId()};
  sai_stat_id_t mplsLookupFailCounterStatId_{kInvalidStatId()};
  sai_stat_id_t aclDropCounterStatId_{kInvalidStatId()};
  sai_stat_id_t trapDropCounterStatId_{kInvalidStatId()};
  sai_stat_id_t egressForwardingDropCounterStatId_{kInvalidStatId()};
  sai_stat_id_t l2SwitchDropCounterStatId_{kInvalidStatId()};
  sai_stat_id_t l3SwitchDropCounterStatId_{kInvalidStatId()};
  sai_stat_id_t tunnelSwitchDropCounterStatId_{kInvalidStatId()};
  sai_stat_id_t srv6MySidSwitchDropCounterStatId_{kInvalidStatId()};
  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  SaiPlatform* platform_;
};

} // namespace facebook::fboss
