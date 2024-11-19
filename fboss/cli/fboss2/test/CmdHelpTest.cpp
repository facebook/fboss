// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/help/CmdHelp.h"
#include <fboss/cli/fboss2/CmdList.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

using namespace ::testing;

namespace facebook::fboss {
class CmdHelpTestFixture : public CmdHandlerTestBase {
 public:
  const CommandTree& cmdTree = kCommandTree();
  const CommandTree& addCmdTree = kAdditionalCommandTree();
  std::vector<CommandTree> cmdTrees = {cmdTree, addCmdTree};

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
  }
};

// Performs test for all commands and all cases.
TEST_F(CmdHelpTestFixture, CmdHelpTest) {
  std::stringstream ss;

  for (const auto& cmdTree : cmdTrees) {
    for (const auto& rootCmd : cmdTree) {
      const auto& objectName = rootCmd.name;
      CmdHelp(cmdTrees).printHelpInfo(objectName, ss);
      std::string output = ss.str();
      const auto& applicableVerb = rootCmd.verb;
      /* testing the first level help separately because the
      first level in the command tree is a rootCommand, but the
      rest of the levels have Command.
      */
      EXPECT_THAT(output, HasSubstr(applicableVerb));

      // testing all nested commands' help info
      /* testing logic for nested sub-commands:
        I am performing an iterative in-order traversal of the command tree
        to check that the help info for each subcommand is displayed.
        This is using a stack.
      */
      for (const auto& subCommand : rootCmd.subcommands) {
        std::stack<Command> subCmdStack;
        subCmdStack.push(subCommand);
        while (!subCmdStack.empty()) {
          auto subCmd = subCmdStack.top();
          subCmdStack.pop();
          const auto& name = subCmd.name;
          EXPECT_THAT(output, HasSubstr(name));
          for (const auto& nestedSubCmds : subCmd.subcommands) {
            subCmdStack.push(nestedSubCmds);
          }
        }
      }
    }
  }
}

} // namespace facebook::fboss
