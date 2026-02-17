/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/session/CmdConfigSessionClear.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <filesystem>
#include <iostream>
#include <ostream>
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace fs = std::filesystem;

namespace facebook::fboss {

CmdConfigSessionClearTraits::RetType CmdConfigSessionClear::queryClient(
    const HostInfo& /* hostInfo */) {
  // Use static path getters to check for session files without calling
  // getInstance(), which would create a session if one doesn't exist
  std::string sessionConfigPath = ConfigSession::getSessionConfigPathStatic();
  std::string metadataPath = ConfigSession::getSessionMetadataPathStatic();

  std::error_code ec;
  bool removedConfig = false;
  bool removedMetadata = false;

  // Remove session config file (~/.fboss2/agent.conf)
  if (fs::exists(sessionConfigPath)) {
    fs::remove(sessionConfigPath, ec);
    if (ec) {
      throw std::runtime_error(
          fmt::format(
              "Failed to remove session config file {}: {}",
              sessionConfigPath,
              ec.message()));
    }
    removedConfig = true;
  }

  // Remove metadata file (~/.fboss2/cli_metadata.json)
  if (fs::exists(metadataPath)) {
    ec.clear();
    fs::remove(metadataPath, ec);
    if (ec) {
      throw std::runtime_error(
          fmt::format(
              "Failed to remove metadata file {}: {}",
              metadataPath,
              ec.message()));
    }
    removedMetadata = true;
  }

  if (removedConfig || removedMetadata) {
    return "Config session cleared successfully.";
  }
  return "No config session exists. Nothing to clear.";
}

void CmdConfigSessionClear::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void
CmdHandler<CmdConfigSessionClear, CmdConfigSessionClearTraits>::run();

} // namespace facebook::fboss
