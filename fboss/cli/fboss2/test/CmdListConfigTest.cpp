/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include "fboss/cli/fboss2/CmdList.h"
#include "fboss/cli/fboss2/CmdSubcommands.h"

namespace facebook::fboss {

// This test verifies that the command trees can be successfully registered
// with CLI11 without throwing CLI::OptionAlreadyAdded exceptions due to
// duplicate subcommand names.
TEST(CmdListConfigTest, noDuplicateSubcommands) {
  CLI::App app{"Test CLI"};

  // This will throw CLI::OptionAlreadyAdded if there are duplicate subcommands
  EXPECT_NO_THROW(
      CmdSubcommands().init(
          app, kCommandTree(), kAdditionalCommandTree(), kSpecialCommands()));
}

} // namespace facebook::fboss
