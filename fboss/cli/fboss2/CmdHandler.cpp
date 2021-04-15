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

#include <folly/Singleton.h>

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
  // TODO add definition

  // Derive IP of the supplied host.
  // Create desired client for the host.
  // Query desired method using the client handle.
  // Print output
}

} // namespace facebook::fboss
