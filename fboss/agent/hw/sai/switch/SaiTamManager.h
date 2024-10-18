// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/sai/api/TamApi.h"
#include "fboss/agent/hw/sai/api/TamEventAgingGroupApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class SaiStore;

using SaiTamCollector = SaiObject<SaiTamCollectorTraits>;
using SaiTamTransport = SaiObject<SaiTamTransportTraits>;
using SaiTamReport = SaiObject<SaiTamReportTraits>;
using SaiTamEventAction = SaiObject<SaiTamEventActionTraits>;
#if defined(SAI_VERSION_11_3_0_0_DNX_ODP)
using SaiTamEventAgingGroup = SaiObject<SaiTamEventAgingGroupTraits>;
#endif
using SaiTamEvent = SaiObject<SaiTamEventTraits>;
using SaiTam = SaiObject<SaiTamTraits>;

struct SaiTamHandle {
  std::shared_ptr<SaiTamCollector> collector;
  std::shared_ptr<SaiTamTransport> transport;
  std::shared_ptr<SaiTamReport> report;
  std::shared_ptr<SaiTamEventAction> action;
#if defined(SAI_VERSION_11_3_0_0_DNX_ODP)
  std::shared_ptr<SaiTamEventAgingGroup> agingGroup;
#endif
  std::shared_ptr<SaiTamEvent> event;
  std::shared_ptr<SaiTam> tam;
  SaiManagerTable* managerTable;
  ~SaiTamHandle();
};
class SaiTamManager {
 public:
  SaiTamManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      SaiPlatform* platform);
  const SaiTamHandle* getTamHandle() const {
    return tamHandle_.get();
  }
  SaiTamHandle* getTamHandle() {
    return tamHandle_.get();
  }

  void gracefulExit() {
    tamHandle_.reset();
  }

 private:
  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  SaiPlatform* platform_;
  std::unique_ptr<SaiTamHandle> tamHandle_;
};

} // namespace facebook::fboss
