// Copyright 2014-present Facebook. All Rights Reserved.

#include <fboss/lib/CommonFileUtils.h>
#include <fboss/platform/data_corral_service/darwin/DarwinFruModule.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss::platform::data_corral_service {

void DarwinFruModule::init(std::vector<AttributeConfig>& attrs) {
  XLOG(DBG2) << "init " << getFruId();
  for (auto attr : attrs) {
    if (*attr.name() == "present") {
      presentPath_ = *attr.path();
    }
  }
  refresh();
}

void DarwinFruModule::refresh() {
  XLOG(DBG4) << "refresh " << getFruId() << " present state is "
             << facebook::fboss::readSysfs(presentPath_) << " after reading "
             << presentPath_;
}

} // namespace facebook::fboss::platform::data_corral_service
