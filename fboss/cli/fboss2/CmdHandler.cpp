/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/CmdHandler.h"

#include "fboss/cli/fboss2/CmdCreateClient.h"
#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/CmdUtils.h"

#include <folly/Singleton.h>
#include <folly/logging/xlog.h>

namespace {
struct singleton_tag_type {};
} // namespace

using facebook::fboss::CmdHandler;
static folly::Singleton<CmdHandler, singleton_tag_type>
    cmdHandler{};
std::shared_ptr<CmdHandler> CmdHandler::getInstance() {
  return cmdHandler.try_get();
}

namespace facebook::fboss {

void CmdHandler::run() {
  // TODO Implements show acl
  // generalize for any subcommand implementation

  // Derive IP of the supplied host.
  auto host = CmdGlobalOptions::getInstance()->getHost();
  auto hostIp = utils::getIPFromHost(host);
  XLOG(DBG0) << "host: " << host << " ip: " << hostIp.str();

  // Create desired client for the host.
  folly::EventBase evb;
  auto client =
      CmdCreateClient::getInstance()
          ->create<facebook::fboss::FbossCtrlAsyncClient>(hostIp.str(), evb);

  // Query desired method using the client handle.
  std::vector<facebook::fboss::AclEntryThrift> aclTable;
  client->sync_getAclTable(aclTable);

  // Print output
  for (auto const& aclEntry : aclTable) {
    std::cout << aclEntry.get_name() << std::endl;
  }
}

} // namespace facebook::fboss
