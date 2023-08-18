// Copyright 2023-present Facebook. All Rights Reserved.

#include <fboss/lib/CommonFileUtils.h>
#include <fboss/platform/data_corral_service/meru800bia/Meru800biaFruModule.h>
#include <folly/logging/xlog.h>
#include <filesystem>

namespace facebook::fboss::platform::data_corral_service {

void Meru800biaFruModule::init(std::vector<AttributeConfig>& attrs) {
  XLOG(DBG2) << "init " << getFruId();
  for (auto attr : attrs) {
    if (*attr.name() == "present") {
      presentPath_ = *attr.path();
    }
  }
  refresh();
}

void Meru800biaFruModule::refresh() {
  if (std::filesystem::exists(std::filesystem::path(presentPath_))) {
    std::string presence = facebook::fboss::readSysfs(presentPath_);
    try {
      isPresent_ = (std::stoi(presence) > 0);
    } catch (const std::exception& ex) {
      XLOG(ERR) << "failed to parse present state from " << presentPath_
                << " where the value is " << presence;
      throw;
    }
    XLOG(DBG4) << "refresh " << getFruId() << " present state is " << isPresent_
               << " after reading " << presentPath_;
  } else {
    XLOG(ERR) << "\"" << presentPath_ << "\""
              << " does not exists";
  }
}

} // namespace facebook::fboss::platform::data_corral_service
