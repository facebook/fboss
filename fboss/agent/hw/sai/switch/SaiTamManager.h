// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/sai/api/TamApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class SaiStore;

using SaiTamReport = SaiObject<SaiTamReportTraits>;
using SaiTamEventAction = SaiObject<SaiTamEventActionTraits>;
using SaiTamEvent = SaiObject<SaiTamEventTraits>;
using SaiTam = SaiObject<SaiTamTraits>;

struct SaiTamHandle {
  std::shared_ptr<SaiTamReport> report;
  std::shared_ptr<SaiTamEventAction> action;
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
