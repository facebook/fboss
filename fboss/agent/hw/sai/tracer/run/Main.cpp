/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <string>
#include <unordered_map>

#include "fboss/agent/hw/sai/tracer/run/saiLog.h"

extern "C" {
#include <sai.h>
}

namespace {

static std::unordered_map<std::string, std::string> kSaiProfileValues{
    {SAI_KEY_BOOT_TYPE, "0"},
    {SAI_KEY_INIT_CONFIG_FILE, "/dev/shm/fboss/hw_config"},
    {SAI_KEY_WARM_BOOT_WRITE_FILE,
     "/dev/shm/fboss/warm_boot/sai_replayer_adaptor_state_0"},
    {SAI_KEY_WARM_BOOT_READ_FILE,
     "/dev/shm/fboss/warm_boot/sai_replayer_adaptor_state_0"}};

const char* saiProfileGetValue(
    sai_switch_profile_id_t /*profile_id*/,
    const char* variable) {
  auto saiProfileValItr = kSaiProfileValues.find(variable);
  return saiProfileValItr != kSaiProfileValues.end()
      ? saiProfileValItr->second.c_str()
      : nullptr;
}

int saiProfileGetNextValue(
    sai_switch_profile_id_t /* profile_id */,
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
  *variable = saiProfileValItr->first.c_str();
  *value = saiProfileValItr->second.c_str();
  ++saiProfileValItr;
  return 0;
}

sai_service_method_table_t kSaiServiceMethodTable = {
    .profile_get_value = saiProfileGetValue,
    .profile_get_next_value = saiProfileGetNextValue,
};

} // namespace

int main() {
  kSaiProfileValues.insert(std::make_pair("SAI_BOOT_TYPE", "0"));
  kSaiProfileValues.insert(
      std::make_pair("SAI_INIT_CONFIG_FILE", "/dev/shm/fboss/hw_config"));
  kSaiProfileValues.insert(std::make_pair(
      "SAI_WARM_BOOT_WRITE_FILE",
      "/dev/shm/fboss/warm_boot/sai_adaptor_state_0"));
  kSaiProfileValues.insert(std::make_pair(
      "SAI_WARM_BOOT_READ_FILE",
      "/dev/shm/fboss/warm_boot/sai_adaptor_state_0"));

  sai_api_initialize(0, &kSaiServiceMethodTable);

  facebook::fboss::run_trace();
}
