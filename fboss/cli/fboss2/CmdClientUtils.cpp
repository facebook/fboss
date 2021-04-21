/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/cli/fboss2/CmdClientUtils.h"

#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/cli/fboss2/CmdUtils.h"

#include <string>

namespace facebook::fboss::utils {

template std::unique_ptr<facebook::fboss::FbossCtrlAsyncClient>
createClient(const std::string& ip);

template <typename T>
std::unique_ptr<T> createClient(const std::string& ip) {
  if constexpr (std::is_same_v<T, facebook::fboss::FbossCtrlAsyncClient>) {
    return utils::createAgentClient(ip);
  } else {
    return utils::createAdditionalClient<T>(ip);
  }
}

} // namespace facebook::fboss::utils

