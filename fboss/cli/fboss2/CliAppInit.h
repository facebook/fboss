/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <CLI/App.hpp>
#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/CmdList.h"
#include "fboss/cli/fboss2/CmdSubcommands.h"

namespace facebook::fboss::utils {

/**
 * Initialize the CLI app with global options and command tree.
 * This sets up the CLI infrastructure and should be called before parsing.
 *
 * This function is shared between the main CLI binary and CLI E2E tests
 * to ensure consistent CLI initialization.
 */
inline void initApp(CLI::App& app) {
  app.require_subcommand();

  // Initialize global options (--fmt, --host, etc.)
  CmdGlobalOptions::getInstance()->init(app);

  // Initialize command tree with all available commands
  CmdSubcommands::getInstance()->init(
      app, kCommandTree(), kAdditionalCommandTree(), kSpecialCommands());
}

} // namespace facebook::fboss::utils
