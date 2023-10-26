// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/CommonFileUtils.h"

#include <folly/Subprocess.h>
#include <folly/system/Shell.h>
#include <exception>

using namespace folly::literals::shell_literals;

namespace facebook::fboss {

void runShellCommand(const std::string& command, bool throwOnError) {
  try {
    auto shellCommand = "{}"_shellify(command);
    runCommand(shellCommand);
  } catch (const std::exception& e) {
    XLOG(ERR) << "Exception while running shell command: " << command << ": "
              << e.what();
    if (throwOnError) {
      throw;
    }
  }
}

void runCommand(const std::vector<std::string>& argv) {
  try {
    folly::Subprocess proc{argv};
    proc.waitChecked();
  } catch (const std::exception& e) {
    XLOG(ERR) << "Exception while running shell command: "
              << folly::join(" ", argv) << ": " << e.what();
    throw;
  }
}

void runAndRemoveScript(
    const std::string& script,
    const std::vector<std::string>& args) {
  if (!checkFileExists(script)) {
    XLOG(DBG2) << "No script found at: " << script;
    return;
  }
  if (!access(script.c_str(), X_OK)) {
    throw FbossError(
        "Execute permissions missing for script found at: ", script);
  }
  auto shellCmd = script + " " + folly::join(" ", args);
  runShellCommand(shellCmd);
  removeFile(script);
}

} // namespace facebook::fboss
