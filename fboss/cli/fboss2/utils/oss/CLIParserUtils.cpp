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
CLI::App* getSubcommandIf(const CLI::App& cmd, const std::string& subcommand) {
  try {
    return cmd.get_subcommand(subcommand);
  } catch (CLI::OptionNotFound& ex) {
    return nullptr;
  }
}
} // namespace facebook::fboss::utils
