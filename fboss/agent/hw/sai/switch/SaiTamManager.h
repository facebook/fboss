// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/sai/api/TamApi.h"
#include "fboss/agent/hw/sai/api/TamEventAgingGroupApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/state/MirrorOnDropReport.h"

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class SaiStore;

using SaiTamCollector = SaiObject<SaiTamCollectorTraits>;
using SaiTamTransport = SaiObject<SaiTamTransportTraits>;
using SaiTamReport = SaiObject<SaiTamReportTraits>;
using SaiTamEventAction = SaiObject<SaiTamEventActionTraits>;
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
using SaiTamEventAgingGroup = SaiObject<SaiTamEventAgingGroupTraits>;
#endif
using SaiTamEvent = SaiObject<SaiTamEventTraits>;
using SaiTam = SaiObject<SaiTamTraits>;

struct SaiTamHandle {
  std::shared_ptr<SaiTamReport> report;
  std::shared_ptr<SaiTamEventAction> action;
  std::shared_ptr<SaiTamTransport> transport;
  std::shared_ptr<SaiTamCollector> collector;
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  std::vector<std::shared_ptr<SaiTamEventAgingGroup>> agingGroups;
#endif
  std::vector<std::shared_ptr<SaiTamEvent>> events;
  std::shared_ptr<SaiTam> tam;
  PortID portId;
  SaiManagerTable* managerTable;
};
class SaiTamManager {
 public:
  SaiTamManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      SaiPlatform* platform);

  void addMirrorOnDropReport(const std::shared_ptr<MirrorOnDropReport>& report);
  void removeMirrorOnDropReport(
      const std::shared_ptr<MirrorOnDropReport>& report);
  void changeMirrorOnDropReport(
      const std::shared_ptr<MirrorOnDropReport>& oldReport,
      const std::shared_ptr<MirrorOnDropReport>& newReport);
  std::vector<PortID> getAllMirrorOnDropPortIds();

  const SaiTamHandle* getTamHandle(const std::string& name) const {
    return tamHandles_.contains(name) ? tamHandles_.at(name).get() : nullptr;
  }
  SaiTamHandle* getTamHandle(const std::string& name) {
    return tamHandles_.contains(name) ? tamHandles_.at(name).get() : nullptr;
  }

  void gracefulExit() {
    tamHandles_.clear();
  }

 private:
  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  SaiPlatform* platform_;
  folly::F14FastMap<std::string, std::unique_ptr<SaiTamHandle>> tamHandles_;
};

} // namespace facebook::fboss
