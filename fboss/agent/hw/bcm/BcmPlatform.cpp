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

#include <folly/FileUtil.h>
#include <folly/String.h>

#include "fboss/agent/SysError.h"
#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/bcm/BcmConfig.h"

DEFINE_string(
    hw_config_file,
    "hw_config",
    "File for dumping HW config on startup");
DEFINE_bool(
    enable_routes_in_host_table,
    false,
    "Whether to program host routes in host table. If false, all "
    "routes are programmed in route table");

namespace facebook::fboss {

std::string BcmPlatform::getHwConfigDumpFile() const {
  return getVolatileStateDir() + "/" + FLAGS_hw_config_file;
}

bool BcmPlatform::isBcmShellSupported() const {
  return true;
}

void BcmPlatform::dumpHwConfig() const {
  std::vector<std::string> nameValStrs;
  for (const auto& kv : BcmAPI::getHwConfig()) {
    nameValStrs.emplace_back(folly::to<std::string>(kv.first, '=', kv.second));
  }
  auto bcmConf = folly::join('\n', nameValStrs);
  auto hwConfigFile = getHwConfigDumpFile();
  if (!folly::writeFile(bcmConf, hwConfigFile.c_str())) {
    throw facebook::fboss::SysError(errno, "error writing bcm config ");
  }
}
} // namespace facebook::fboss
