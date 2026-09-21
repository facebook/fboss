// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/utils/CmdInitUtils.h"

#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/CmdList.h"
#include "fboss/cli/fboss2/CmdSubcommands.h"

namespace facebook::fboss::utils {
void initApp(CLI::App& app) {
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
}
} // namespace facebook::fboss::utils
