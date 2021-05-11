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

std::unique_ptr<facebook::fboss::FbossCtrlAsyncClient> createAgentClient(
    const std::string& ip) {
  auto agentPort = CmdGlobalOptions::getInstance()->getAgentThriftPort();
  return createPlaintextClient<facebook::fboss::FbossCtrlAsyncClient>(
      ip, agentPort);
}

std::unique_ptr<facebook::fboss::QsfpServiceAsyncClient> createQsfpClient(
    const std::string& ip) {
  auto qsfpServicePort = CmdGlobalOptions::getInstance()->getQsfpThriftPort();
  return createPlaintextClient<facebook::fboss::QsfpServiceAsyncClient>(
      ip, qsfpServicePort);
}

} // namespace facebook::fboss::utils
