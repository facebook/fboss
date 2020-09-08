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

namespace facebook::fboss {

SaiHandler::SaiHandler(SwSwitch* sw, const SaiSwitch* hw)
    : ThriftHandler(sw), hw_(hw), diagShell_(hw) {}

SaiHandler::~SaiHandler() {}

apache::thrift::ResponseAndServerStream<std::string, std::string>
SaiHandler::startDiagShell() {
  XLOG(INFO) << "New diag shell session connecting";
  if (!hw_->isFullyInitialized()) {
    throw FbossError("switch is still initializing or is exiting ");
  }
  if (diagShell_.hasPublisher()) {
    throw FbossError("Diag shell already connected");
  }
  auto streamAndPublisher =
      apache::thrift::ServerStream<std::string>::createPublisher([this]() {
        diagShell_.markResetPublisher();
        XLOG(INFO) << "Diag shell session disconnected";
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
    folly::fbstring& /* unused */,
    std::unique_ptr<fbstring> /* unused */,
    std::unique_ptr<ClientInformation> /* unused */) {
  // TODO
  throw FbossError("Unsupported");
}

} // namespace facebook::fboss
