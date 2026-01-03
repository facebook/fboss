/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/session/CmdConfigSessionDiff.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"

#include <folly/Subprocess.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace facebook::fboss {

namespace {

// Helper function to resolve a revision specifier to a file path
// Note: Revision format validation is done in RevisionList constructor
std::string resolveRevisionPath(
    const std::string& revision,
    const std::string& cliConfigDir,
    const std::string& systemConfigPath) {
  if (revision == "current") {
    return systemConfigPath;
  }

  // Build the path (revision is already validated to be in "rN" format)
  std::string revisionPath = cliConfigDir + "/agent-" + revision + ".conf";

  // Check if the file exists
  if (!fs::exists(revisionPath)) {
    throw std::invalid_argument(
        "Revision " + revision + " does not exist at " + revisionPath);
  }

  return revisionPath;
}

// Helper function to execute diff and return the result
std::string executeDiff(
    const std::string& path1,
    const std::string& path2,
    const std::string& label1,
    const std::string& label2) {
  try {
    folly::Subprocess proc(
        std::vector<std::string>{
            "/usr/bin/diff",
            "-u",
            "--label",
            label1,
            "--label",
            label2,
            path1,
            path2},
        folly::Subprocess::Options().pipeStdout().pipeStderr());

    auto result = proc.communicate();
    int returnCode = proc.wait().exitStatus();

    // diff returns 0 if files are identical, 1 if different, 2 on error
    if (returnCode == 0) {
      return "No differences between " + label1 + " and " + label2 + ".";
    } else if (returnCode == 1) {
      // Files differ - return the diff output
      return result.first;
    } else {
      // Error occurred
      throw std::runtime_error("diff command failed: " + result.second);
    }
  } catch (const std::exception& ex) {
    throw std::runtime_error(
        "Failed to execute diff command: " + std::string(ex.what()));
  }
}

} // namespace

CmdConfigSessionDiffTraits::RetType CmdConfigSessionDiff::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::RevisionList& revisions) {
  auto& session = ConfigSession::getInstance();

  std::string systemConfigPath = session.getSystemConfigPath();
  std::string sessionConfigPath = session.getSessionConfigPath();
  std::string cliConfigDir = session.getCliConfigDir();

  // Mode 1: No arguments - diff session vs current live config
  if (revisions.empty()) {
    if (!session.sessionExists()) {
      return "No config session exists. Make a config change first.";
    }

    return executeDiff(
        systemConfigPath,
        sessionConfigPath,
        "current live config",
        "session config");
  }

  // Mode 2: One argument - diff session vs specified revision
  if (revisions.size() == 1) {
    if (!session.sessionExists()) {
      return "No config session exists. Make a config change first.";
    }

    std::string revisionPath =
        resolveRevisionPath(revisions[0], cliConfigDir, systemConfigPath);
    std::string label =
        revisions[0] == "current" ? "current live config" : revisions[0];

    return executeDiff(
        revisionPath, sessionConfigPath, label, "session config");
  }

  // Mode 3: Two arguments - diff between two revisions
  if (revisions.size() == 2) {
    std::string path1 =
        resolveRevisionPath(revisions[0], cliConfigDir, systemConfigPath);
    std::string path2 =
        resolveRevisionPath(revisions[1], cliConfigDir, systemConfigPath);

    std::string label1 =
        revisions[0] == "current" ? "current live config" : revisions[0];
    std::string label2 =
        revisions[1] == "current" ? "current live config" : revisions[1];

    return executeDiff(path1, path2, label1, label2);
  }

  // More than 2 arguments is an error
  throw std::invalid_argument(
      "Too many arguments. Expected 0, 1, or 2 revision specifiers.");
}

void CmdConfigSessionDiff::printOutput(const RetType& diffOutput) {
  std::cout << diffOutput;
  if (!diffOutput.empty() && diffOutput.back() != '\n') {
    std::cout << std::endl;
  }
}

} // namespace facebook::fboss
