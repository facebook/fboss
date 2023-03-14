/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

namespace facebook::fboss::utils {

template <>
std::unique_ptr<facebook::fboss::FbossCtrlAsyncClient> createClient(
    const HostInfo& hostInfo) {
  return utils::createAgentClient(hostInfo);
}

template <>
std::unique_ptr<facebook::fboss::QsfpServiceAsyncClient> createClient(
    const HostInfo& hostInfo) {
  return utils::createQsfpClient(hostInfo);
}

} // namespace facebook::fboss::utils
