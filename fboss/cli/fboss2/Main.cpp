/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <CLI/CLI.hpp>
#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/CmdSubcommands.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

#include <folly/init/Init.h>
#include <folly/logging/Init.h>
#include <folly/logging/xlog.h>

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG0; default:async=true");

namespace facebook::fboss {

int cliMain(int argc, char* argv[]) {
  int one = 1;
  folly::init(&one, &argv, /* removeFlags = */ false);

  CLI::App app{"FBOSS CLI"};

  app.require_subcommand();

  /*
   * initialize available global options for CLI
   */
  CmdGlobalOptions::getInstance()->init(app);

  /*
   * initialize/build CLI command token trees
   *
   * NOTE: kCommandTree/kAdditionalCommandTree/kSpecialCommands will be linked
   * from elsewhere to make `CmdSubcommands` an independent lib.
   */
  CmdSubcommands::getInstance()->init(
      app, kCommandTree(), kAdditionalCommandTree(), kSpecialCommands());

  utils::postAppInit(argc, argv, app);

  CLI11_PARSE(app, argc, argv);

  return 0;
}

} // namespace facebook::fboss

int main(int argc, char* argv[]) {
  return facebook::fboss::cliMain(argc, argv);
}
