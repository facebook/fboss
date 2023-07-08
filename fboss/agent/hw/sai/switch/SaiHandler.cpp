/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/switch/SaiHandler.h"

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

#include <folly/logging/xlog.h>
#include "fboss/lib/LogThriftCall.h"

namespace facebook::fboss {

SaiHandler::SaiHandler(SwSwitch* sw, const SaiSwitch* hw)
    : ThriftHandler(sw),
      hw_(hw),
      diagShell_(hw),
      diagCmdServer_(hw, &diagShell_) {}

SaiHandler::~SaiHandler() {}

apache::thrift::ResponseAndServerStream<std::string, std::string>
SaiHandler::startDiagShell() {
  XLOG(DBG2) << "New diag shell session connecting";
  if (!hw_->isFullyInitialized()) {
    throw FbossError("switch is still initializing or is exiting ");
  }
  diagShell_.tryConnect();
  auto streamAndPublisher =
      apache::thrift::ServerStream<std::string>::createPublisher([this]() {
        diagShell_.markResetPublisher();
        XLOG(DBG2) << "Diag shell session disconnected";
      });

  std::string firstPrompt =
      diagShell_.start(std::move(streamAndPublisher.second));
  return {firstPrompt, std::move(streamAndPublisher.first)};
}

void SaiHandler::produceDiagShellInput(
    std::unique_ptr<std::string> input,
    std::unique_ptr<ClientInformation> client) {
  diagShell_.consumeInput(std::move(input), std::move(client));
}

void SaiHandler::diagCmd(
    fbstring& result,
    std::unique_ptr<fbstring> cmd,
    std::unique_ptr<ClientInformation> client,
    int16_t /* unused */,
    bool /* unused */) {
  auto log = LOG_THRIFT_CALL(WARN, *cmd);
  hw_->ensureConfigured(__func__);
  result = diagCmdServer_.diagCmd(std::move(cmd), std::move(client));
}

} // namespace facebook::fboss
