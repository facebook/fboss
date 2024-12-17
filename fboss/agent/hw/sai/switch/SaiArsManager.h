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

#include "fboss/agent/hw/sai/api/ArsApi.h"
#include "fboss/agent/hw/sai/api/Types.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/state/FlowletSwitchingConfig.h"
#include "fboss/agent/types.h"

#include <memory>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
struct SaiNextHopGroupHandle;
class SaiStore;

#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
using SaiArs = SaiObject<SaiArsTraits>;
#endif

struct SaiArsHandle {
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
  std::shared_ptr<SaiArs> ars;
#endif
};

class SaiArsManager {
 public:
  SaiArsManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
  void addArs(
      const std::shared_ptr<FlowletSwitchingConfig>& flowletSwitchingConfig);
  void removeArs(
      const std::shared_ptr<FlowletSwitchingConfig>& flowletSwitchingConfig);
  void changeArs(
      const std::shared_ptr<FlowletSwitchingConfig>& oldFlowletSwitchingConfig,
      const std::shared_ptr<FlowletSwitchingConfig>& newFlowletSwitchingConfig);
  SaiArsHandle* getArsHandle();
  sai_int32_t cfgSwitchingModeToSai(cfg::SwitchingMode switchingMode) const;
  bool isFlowsetTableFull(ArsSaiId arsSaiId);
#endif

 private:
  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;

#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
  std::unique_ptr<SaiArsHandle> arsHandle_;
#endif
};

} // namespace facebook::fboss
