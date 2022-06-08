// Copyright 2014-present Facebook. All Rights Reserved.

#include <fboss/platform/data_corral_service/darwin/DarwinFanModule.h>

namespace facebook::fboss::platform::data_corral_service {

void DarwinFanModule::refresh() {
  // update isPresent by reading Sysfs path
  // if necessary, use state machine here to help processing
  // detected state changes
}

} // namespace facebook::fboss::platform::data_corral_service
