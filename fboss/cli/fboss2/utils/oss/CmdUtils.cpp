/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss::utils {

std::vector<std::string> getHostsInSmcTier(
    const std::string& /* parentTierName */) {
  return {};
}

const std::string getOobNameFromHost(const std::string& /* host */) {
  return "";
}

void logUsage(const std::string& /*cmdName*/) {}

} // namespace facebook::fboss::utils
