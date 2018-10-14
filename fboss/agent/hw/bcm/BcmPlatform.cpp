/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmPlatform.h"

DEFINE_string(hw_config_file, "hw_config",
              "File for dumping HW config on startup");
DEFINE_bool(enable_routes_in_host_table,
            false,
            "Whether to program host routes in host table. If false, all "
            "routes are programmed in route table");
DEFINE_string(script_pre_asic_init, "script_pre_asic_init",
              "Broadcom script file to be run before ASIC init");

namespace facebook { namespace fboss {

BcmPlatform::BcmPlatform() {}

std::string BcmPlatform::getHwConfigDumpFile() const {
  return getVolatileStateDir() + "/" + FLAGS_hw_config_file;
}

std::string BcmPlatform::getScriptPreAsicInit() const {
  return getPersistentStateDir() + "/" + FLAGS_script_pre_asic_init;
}

bool BcmPlatform::isBcmShellSupported() const {
  return true;
}

}} //facebook::fboss
