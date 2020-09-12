/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/hw_test/SaiTestHandler.h"

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

#include <folly/logging/xlog.h>

using folly::fbstring;

namespace facebook::fboss {

SaiTestHandler::SaiTestHandler(const SaiSwitch* hw)
    : FacebookBase2("FBOSS_SAI_TEST"), hw_(hw), diagShell_(hw) {}

SaiTestHandler::~SaiTestHandler() {}

apache::thrift::ResponseAndServerStream<std::string, std::string>
SaiTestHandler::startDiagShell() {
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

void SaiTestHandler::produceDiagShellInput(
    std::unique_ptr<std::string> input,
    std::unique_ptr<ClientInformation> client) {
  diagShell_.consumeInput(std::move(input), std::move(client));
}

void SaiTestHandler::diagCmd(
    folly::fbstring& /* unused */,
    std::unique_ptr<fbstring> /* unused */,
    std::unique_ptr<ClientInformation> /* unused */) {
  // TODO
  throw FbossError("Unsupported");
}

} // namespace facebook::fboss
