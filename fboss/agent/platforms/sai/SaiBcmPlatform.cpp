/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmPlatform.h"

#include <cstring>
#include <cstdio>

extern "C" {
#include <sai.h>
#include <saistatus.h>
#include <saiswitch.h>
}

namespace {
const std::unordered_map<const char*, const char*> kSaiProfileValues = {
  // FIXME - don't hard code the config.bcm path
  {SAI_KEY_INIT_CONFIG_FILE , "/root/config.bcm"},
  // COLD boot only for now
  {SAI_KEY_BOOT_TYPE, "0"},
};

const char* saiProfileGetValue(sai_switch_profile_id_t /*profile_id*/,
    const char* variable) {
  auto saiProfileValItr = kSaiProfileValues.find(variable);
  return saiProfileValItr != kSaiProfileValues.end() ? saiProfileValItr->second
                                                     : nullptr;
}

int saiProfileGetNextValue(
        sai_switch_profile_id_t profile_id,
         const char** variable,
         const char** value) {
  static auto saiProfileValItr = kSaiProfileValues.begin();
  if (!value) {
    saiProfileValItr = kSaiProfileValues.begin();
    return 0;
  }
  if (saiProfileValItr == kSaiProfileValues.end()) {
    return -1;
  }
  *variable = saiProfileValItr->first;
  *value = saiProfileValItr->second;
  ++saiProfileValItr;
  return 0;
}

sai_service_method_table_t kSaiServiceMethodTable = {
 .profile_get_value = saiProfileGetValue,
 .profile_get_next_value = saiProfileGetNextValue,
};

}

namespace facebook { namespace fboss {

sai_service_method_table_t* SaiBcmPlatform::getServiceMethodTable() const {
  return &kSaiServiceMethodTable;
}

}}
