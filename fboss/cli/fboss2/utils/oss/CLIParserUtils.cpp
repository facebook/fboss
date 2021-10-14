/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/utils/CLIParserUtils.h"

namespace facebook::fboss::utils {
CLI::App* getSubcommandIf(CLI::App& cmd, const std::string& subcommand) {
  auto subcommands = cmd.get_subcommands([subcommand](CLI::App* app) {
    return app->get_name().compare(subcommand) == 0;
  });

  return subcommands.empty() ? nullptr : subcommands.front();
}
} // namespace facebook::fboss::utils
