// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/utils/SafetyPromptUtils.h"

#include "fboss/cli/fboss2/CmdLocalOptions.h"

#include <fmt/format.h>
#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include <string>

namespace facebook::fboss::utils {

namespace {
bool isInteractiveTty() {
  return ::isatty(::fileno(stdin)) != 0 && ::isatty(::fileno(stderr)) != 0;
}
} // namespace

void requireConfirmation(
    const std::string& commandName,
    const std::string& yesFlagName,
    const std::string& warning,
    const std::string& target) {
  const bool skip = CmdLocalOptions::getInstance()->getLocalOption(
                        commandName, yesFlagName) == "true";
  if (skip) {
    return;
  }

  if (!isInteractiveTty()) {
    throw std::invalid_argument(
        "Refusing to proceed: stdin/stderr is not a TTY and -y/--yes was not "
        "passed. Re-run interactively (after reviewing the warning) or pass "
        "-y to acknowledge the risk. Prefer `cableguy_cli bounce_interface` "
        "for production work — it validates drain state before bouncing.");
  }

  std::cerr << warning;
  std::cerr << fmt::format(
      "\nProceed for: {}\nType 'yes' to continue, anything else aborts: ",
      target);

  std::string answer;
  if (!std::getline(std::cin, answer) || (answer != "yes" && answer != "YES")) {
    throw std::invalid_argument("Aborted by user. No state was changed.");
  }
}

} // namespace facebook::fboss::utils
