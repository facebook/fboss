// Copyright 2014-present Facebook. All Rights Reserved.

#include <fboss/platform/data_corral_service/darwin/DarwinRackmonModule.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss::platform::data_corral_service {

void DarwinRackmonModule::init() {
  // TODO(daiweix): if necessary, use state machine here to help processing
  // detected state changes
  XLOG(DBG2) << "init " << getName();
}

void DarwinRackmonModule::refresh() {
  // TODO(daiweix): update isPresent by reading Sysfs path
  // if necessary, use state machine here to help processing
  // detected state changes
  XLOG(DBG4) << "refresh " << getName();
}

std::string DarwinRackmonModule::getName() {
  return "DarwinRackmonModule-" + std::to_string(id_);
}

} // namespace facebook::fboss::platform::data_corral_service
