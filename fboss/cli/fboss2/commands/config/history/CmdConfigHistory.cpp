/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/history/CmdConfigHistory.h"
#include <ctime>
#include <sstream>
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

namespace {

// Format Unix timestamp (seconds) as a human-readable string
std::string formatTime(int64_t timeSec) {
  char buffer[32];
  tm timeinfo{};
  std::time_t time = timeSec;
  localtime_r(&time, &timeinfo);
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return buffer;
}

} // namespace

CmdConfigHistoryTraits::RetType CmdConfigHistory::queryClient(
    const HostInfo& /* hostInfo */) {
  auto& session = ConfigSession::getInstance();
  auto& git = session.getGit();

  // Get the commit history from Git for the CLI config file
  auto commits = git.log(session.getCliConfigPath());

  if (commits.empty()) {
    return "No config revisions found in Git history";
  }

  // Build the table
  utils::Table table;
  table.setHeader({"Commit", "Author", "Commit Time", "Message"});

  for (const auto& commit : commits) {
    // Use short SHA1 (first 8 characters)
    std::string shortSha = commit.sha1.substr(0, 8);
    table.addRow(
        {shortSha,
         commit.authorName,
         formatTime(commit.timestamp),
         commit.subject});
  }

  // Convert table to string
  std::ostringstream oss;
  oss << table;
  return oss.str();
}

void CmdConfigHistory::printOutput(const RetType& tableOutput) {
  std::cout << tableOutput << std::endl;
}

} // namespace facebook::fboss
