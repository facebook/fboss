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
#include <fcntl.h>
#include <pwd.h>
#include <sys/stat.h>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <vector>
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace fs = std::filesystem;

namespace facebook::fboss {

namespace {

struct RevisionInfo {
  int revisionNumber;
  std::string owner;
  int64_t commitTimeNsec; // Commit time in nanoseconds since epoch
  std::string filePath;
};

// Get the username from a UID
std::string getUsername(uid_t uid) {
  struct passwd* pw = getpwuid(uid);
  if (pw) {
    return std::string(pw->pw_name);
  }
  // If we can't resolve the username, return the UID as a string
  return "UID:" + std::to_string(uid);
}

// Format time as a human-readable string with milliseconds
std::string formatTime(int64_t timeNsec) {
  // Convert nanoseconds to seconds and remaining nanoseconds
  std::time_t timeSec = timeNsec / 1000000000;
  long nsec = timeNsec % 1000000000;

  char buffer[100];
  struct tm* timeinfo = std::localtime(&timeSec);
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

  // Add milliseconds
  long milliseconds = nsec / 1000000;
  std::ostringstream oss;
  oss << buffer << '.' << std::setfill('0') << std::setw(3) << milliseconds;
  return oss.str();
}

// Collect all revision files from the CLI config directory
std::vector<RevisionInfo> collectRevisions(const std::string& cliConfigDir) {
  std::vector<RevisionInfo> revisions;

  std::error_code ec;
  if (!fs::exists(cliConfigDir, ec) || !fs::is_directory(cliConfigDir, ec)) {
    // Directory doesn't exist or is not a directory
    return revisions;
  }

  for (const auto& entry : fs::directory_iterator(cliConfigDir, ec)) {
    if (ec) {
      continue; // Skip entries we can't read
    }

    if (!entry.is_regular_file(ec)) {
      continue; // Skip non-regular files
    }

    std::string filename = entry.path().filename().string();
    int revNum = ConfigSession::extractRevisionNumber(filename);

    if (revNum < 0) {
      continue; // Skip files that don't match our pattern
    }

    // Get file metadata using statx to get birth time (creation time)
    struct statx stx;
    if (statx(
            AT_FDCWD, entry.path().c_str(), 0, STATX_BTIME | STATX_UID, &stx) !=
        0) {
      continue; // Skip if we can't get file stats
    }

    RevisionInfo info;
    info.revisionNumber = revNum;
    info.owner = getUsername(stx.stx_uid);
    // Use birth time (creation time) if available, otherwise fall back to mtime
    if (stx.stx_mask & STATX_BTIME) {
      info.commitTimeNsec =
          static_cast<int64_t>(stx.stx_btime.tv_sec) * 1000000000 +
          stx.stx_btime.tv_nsec;
    } else {
      info.commitTimeNsec =
          static_cast<int64_t>(stx.stx_mtime.tv_sec) * 1000000000 +
          stx.stx_mtime.tv_nsec;
    }
    info.filePath = entry.path().string();

    revisions.push_back(info);
  }

  // Sort by revision number (ascending)
  std::sort(
      revisions.begin(),
      revisions.end(),
      [](const RevisionInfo& a, const RevisionInfo& b) {
        return a.revisionNumber < b.revisionNumber;
      });

  return revisions;
}

} // namespace

CmdConfigHistoryTraits::RetType CmdConfigHistory::queryClient(
    const HostInfo& hostInfo) {
  auto& session = ConfigSession::getInstance();
  const std::string cliConfigDir = session.getCliConfigDir();

  auto revisions = collectRevisions(cliConfigDir);

  if (revisions.empty()) {
    return "No config revisions found in " + cliConfigDir;
  }

  // Build the table
  utils::Table table;
  table.setHeader({"Revision", "Owner", "Commit Time"});

  for (const auto& rev : revisions) {
    table.addRow(
        {"r" + std::to_string(rev.revisionNumber),
         rev.owner,
         formatTime(rev.commitTimeNsec)});
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
