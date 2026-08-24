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
#include "fboss/cli/fboss2/utils/CLIParserUtils.h"

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

  auto* config = utils::getSubcommandIf(app, "config");
  ASSERT_NE(config, nullptr);
  auto* configSrv6 = utils::getSubcommandIf(*config, "srv6");
  ASSERT_NE(configSrv6, nullptr);
  auto* configMySid = utils::getSubcommandIf(*configSrv6, "my-sid");
  ASSERT_NE(configMySid, nullptr);
  EXPECT_NE(utils::getSubcommandIf(*configMySid, "entry"), nullptr);
  EXPECT_EQ(utils::getSubcommandIf(*configMySid, "add"), nullptr);
  EXPECT_EQ(utils::getSubcommandIf(*configMySid, "delete"), nullptr);

  auto* deleteCmd = utils::getSubcommandIf(app, "delete");
  ASSERT_NE(deleteCmd, nullptr);
  auto* deleteSrv6 = utils::getSubcommandIf(*deleteCmd, "srv6");
  ASSERT_NE(deleteSrv6, nullptr);
  auto* deleteMySid = utils::getSubcommandIf(*deleteSrv6, "my-sid");
  ASSERT_NE(deleteMySid, nullptr);
  EXPECT_NE(utils::getSubcommandIf(*deleteMySid, "entry"), nullptr);
}

} // namespace facebook::fboss
