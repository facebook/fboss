// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/utils/CmdInitUtils.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace facebook::fboss::utils {

namespace {
// Walk the already-built CLI11 command tree following the tokens the user has
// typed so far, then print the names of the sub-commands available at that
// point (one per line). This is the OSS equivalent of internal FastCLI's
// `__metadata` endpoint: the shell completion script queries the binary itself,
// so completion is always in lockstep with the commands compiled into it.
void printCompletions(
    const CLI::App& app,
    const std::vector<std::string>& typedPath) {
  const CLI::App* current = &app;
  for (const auto& token : typedPath) {
    const CLI::App* next = nullptr;
    for (const auto* sub : current->get_subcommands(nullptr)) {
      if (sub->get_name() == token) {
        next = sub;
        break;
      }
    }
    // Token doesn't resolve to a known sub-command (e.g. it's a positional arg
    // or a partial word); nothing further to complete.
    if (next == nullptr) {
      return;
    }
    current = next;
  }
  for (const auto* sub : current->get_subcommands(nullptr)) {
    std::cout << sub->get_name() << "\n";
  }
}
} // namespace

void postAppInit(int argc, char* argv[], CLI::App& app) {
  if (argc >= 2 && std::string(argv[1]) == "__completion") {
    std::vector<std::string> typedPath;
    for (int i = 2; i < argc; ++i) {
      typedPath.emplace_back(argv[i]);
    }
    printCompletions(app, typedPath);
    std::exit(0);
  }
}

} // namespace facebook::fboss::utils
