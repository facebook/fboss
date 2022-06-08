// Copyright 2014-present Facebook. All Rights Reserved.

#include <fboss/platform/data_corral_service/darwin/DarwinChassisManager.h>

namespace facebook::fboss::platform::data_corral_service {

void DarwinChassisManager::initFruModules() {
  // instantiate Fan modules and PSU modules
}

void DarwinChassisManager::programChassis() {
  bool allPresent = true;
  for (auto fruModule : fruModules_) {
    if (!fruModule.isPresent()) {
      allPresent = false;
    }
  }
  // program system LED based on allPresent
}

} // namespace facebook::fboss::platform::data_corral_service
