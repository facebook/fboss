// Copyright 2014-present Facebook. All Rights Reserved.

#include <fboss/platform/data_corral_service/darwin/DarwinPemModule.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss::platform::data_corral_service {

void DarwinPemModule::init() {
  // TODO(daiweix): if necessary, use state machine here to help processing
  // detected state changes
  XLOG(DBG2) << "init " << getName();
}

void DarwinPemModule::refresh() {
  // TODO(daiweix): update isPresent by reading Sysfs path
  // if necessary, use state machine here to help processing
  // detected state changes
  XLOG(DBG4) << "refresh " << getName();
}

std::string DarwinPemModule::getName() {
  return "DarwinPemModule-" + std::to_string(id_);
}

} // namespace facebook::fboss::platform::data_corral_service
