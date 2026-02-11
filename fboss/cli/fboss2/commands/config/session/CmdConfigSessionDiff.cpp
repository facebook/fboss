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

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include "fboss/cli/fboss2/session/ConfigSession.h"

#include <folly/FileUtil.h>
#include <folly/Subprocess.h>

namespace facebook::fboss {

namespace {

// Helper function to get config content from a revision specifier
// Returns the content and a label for the revision
std::pair<std::string, std::string> getRevisionContent(
    const std::string& revision,
    ConfigSession& session) {
  auto& git = session.getGit();
  std::string cliConfigPath = session.getCliConfigPath();

  if (revision == "current") {
    // Read the current live config (via the symlink or directly from cli path)
    std::string content;
    if (!folly::readFile(cliConfigPath.c_str(), content)) {
      throw std::runtime_error(
          "Failed to read current config from " + cliConfigPath);
    }
    return {content, "current live config"};
  }

  // Resolve the commit SHA and get the content from Git
  std::string resolvedSha = git.resolveRef(revision);
  std::string content = git.fileAtRevision(resolvedSha, "cli/agent.conf");
  return {content, revision.substr(0, 8)};
}

// Helper function to execute diff on two strings and return the result
std::string executeDiff(
    const std::string& content1,
    const std::string& content2,
    const std::string& label1,
    const std::string& label2) {
  try {
    // Write content to temporary files for diff
    std::string tmpFile1 = "/tmp/fboss2_diff_1_XXXXXX";
    std::string tmpFile2 = "/tmp/fboss2_diff_2_XXXXXX";

    int fd1 = mkstemp(tmpFile1.data());
    int fd2 = mkstemp(tmpFile2.data());

    if (fd1 < 0 || fd2 < 0) {
      throw std::runtime_error("Failed to create temporary files for diff");
    }

    // Write content and close files
    folly::writeFull(fd1, content1.data(), content1.size());
    folly::writeFull(fd2, content2.data(), content2.size());
    close(fd1);
    close(fd2);

    // Run diff
    folly::Subprocess proc(
        std::vector<std::string>{
            "/usr/bin/diff",
            "-u",
            "--label",
            label1,
            "--label",
            label2,
            tmpFile1,
            tmpFile2},
        folly::Subprocess::Options().pipeStdout().pipeStderr());

    auto result = proc.communicate();
    int returnCode = proc.wait().exitStatus();

    // Clean up temp files
    unlink(tmpFile1.c_str());
    unlink(tmpFile2.c_str());

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

  // Mode 1: No arguments - diff session vs current live config
  if (revisions.empty()) {
    if (!session.sessionExists()) {
      return "No config session exists. Make a config change first.";
    }

    std::string currentContent;
    if (!folly::readFile(systemConfigPath.c_str(), currentContent)) {
      throw std::runtime_error(
          "Failed to read current config from " + systemConfigPath);
    }

    std::string sessionContent;
    if (!folly::readFile(sessionConfigPath.c_str(), sessionContent)) {
      throw std::runtime_error(
          "Failed to read session config from " + sessionConfigPath);
    }

    return executeDiff(
        currentContent,
        sessionContent,
        "current live config",
        "session config");
  }

  // Mode 2: One argument - diff session vs specified revision
  if (revisions.size() == 1) {
    if (!session.sessionExists()) {
      return "No config session exists. Make a config change first.";
    }

    auto [revContent, revLabel] = getRevisionContent(revisions[0], session);

    std::string sessionContent;
    if (!folly::readFile(sessionConfigPath.c_str(), sessionContent)) {
      throw std::runtime_error(
          "Failed to read session config from " + sessionConfigPath);
    }

    return executeDiff(revContent, sessionContent, revLabel, "session config");
  }

  // Mode 3: Two arguments - diff between two revisions
  if (revisions.size() == 2) {
    auto [content1, label1] = getRevisionContent(revisions[0], session);
    auto [content2, label2] = getRevisionContent(revisions[1], session);

    return executeDiff(content1, content2, label1, label2);
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

// Explicit template instantiation
template void
CmdHandler<CmdConfigSessionDiff, CmdConfigSessionDiffTraits>::run();

} // namespace facebook::fboss
