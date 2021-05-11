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
#include "fboss/qsfp_service/if/gen-cpp2/QsfpService.h"

namespace facebook::fboss::utils {

std::unique_ptr<facebook::fboss::FbossCtrlAsyncClient>
createPlaintextAgentClient(const std::string& ip);

std::unique_ptr<facebook::fboss::QsfpServiceAsyncClient>
createPlaintextQsfpClient(const std::string& ip);

std::unique_ptr<facebook::fboss::FbossCtrlAsyncClient> createAgentClient(
    const std::string& ip) {
  return createPlaintextAgentClient(ip);
}

std::unique_ptr<facebook::fboss::QsfpServiceAsyncClient> createQsfpClient(
    const std::string& ip) {
  return createPlaintextQsfpClient(ip);
}

} // namespace facebook::fboss::utils
