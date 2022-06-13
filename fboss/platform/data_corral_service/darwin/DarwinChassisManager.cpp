// Copyright 2014-present Facebook. All Rights Reserved.

#include <fboss/platform/data_corral_service/darwin/DarwinChassisManager.h>
#include <fboss/platform/data_corral_service/darwin/DarwinFanModule.h>
#include <fboss/platform/data_corral_service/darwin/DarwinPemModule.h>
#include <fboss/platform/data_corral_service/darwin/DarwinRackmonModule.h>
#include <folly/logging/xlog.h>

namespace {
int kNumFans = 5;
}

namespace facebook::fboss::platform::data_corral_service {

void DarwinChassisManager::initFruModules() {
  XLOG(DBG2) << "instantiate fru modules";
  for (int i = 0; i < kNumFans; i++) {
    fruModules_.push_back(std::make_unique<DarwinFanModule>(i));
  }
  fruModules_.push_back(std::make_unique<DarwinPemModule>(0));
  fruModules_.push_back(std::make_unique<DarwinRackmonModule>(0));
  for (auto& fruModule : fruModules_) {
    fruModule->init();
  }
}

void DarwinChassisManager::programChassis() {
  XLOG(DBG4) << "program Darwin system LED based on updated Fru module state";
}

} // namespace facebook::fboss::platform::data_corral_service
