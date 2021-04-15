/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/CmdCreateClient.h"

#include "fboss/cli/fboss2/CmdUtils.h"

#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"

#include <folly/Singleton.h>

namespace {
struct singleton_tag_type {};
} // namespace

using facebook::fboss::CmdCreateClient;
static folly::Singleton<CmdCreateClient, singleton_tag_type>
    cmdCreateClient{};
std::shared_ptr<CmdCreateClient> CmdCreateClient::getInstance() {
  return cmdCreateClient.try_get();
}

namespace facebook::fboss {

template <typename T>
std::unique_ptr<T> CmdCreateClient::create(
    const std::string& ip,
    folly::EventBase& evb) {
  if constexpr (std::is_same_v<T, facebook::fboss::FbossCtrlAsyncClient>) {
    return utils::createAgentClient(ip, evb);
  }
}

} // namespace facebook::fboss
