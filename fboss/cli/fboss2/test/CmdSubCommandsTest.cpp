// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <CLI/CLI.hpp>

#include "fboss/cli/fboss2/CmdList.h"
#include "fboss/cli/fboss2/CmdSubcommands.h"
#include "fboss/cli/fboss2/utils/CLIParserUtils.h"

namespace facebook::fboss {
namespace {

class CmdSubCommandsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    app_ = std::make_unique<CLI::App>("Test CLI");
  }

  std::unique_ptr<CLI::App> app_;
};

TEST_F(CmdSubCommandsTest, InitWithEmptyCommandTrees) {
  CommandTree emptyTree;
  std::vector<Command> emptySpecialCmds;

  // Should not throw when initialized with empty trees
  EXPECT_NO_THROW(
      CmdSubcommands::getInstance()->init(
          *app_, emptyTree, emptyTree, emptySpecialCmds));

  // Verify that supported verbs are registered
  EXPECT_NE(utils::getSubcommandIf(*app_, "show"), nullptr);
  EXPECT_NE(utils::getSubcommandIf(*app_, "clear"), nullptr);
  EXPECT_NE(utils::getSubcommandIf(*app_, "create"), nullptr);
  EXPECT_NE(utils::getSubcommandIf(*app_, "delete"), nullptr);
  EXPECT_NE(utils::getSubcommandIf(*app_, "set"), nullptr);
}

TEST_F(CmdSubCommandsTest, InitWithValidCommandTree) {
  CommandTree cmdTree = {
      RootCommand(
          "show",
          "testObject",
          "Test object help",
          []() {},
          []() { return utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE; }),
  };
  CommandTree emptyTree;
  std::vector<Command> emptySpecialCmds;

  EXPECT_NO_THROW(
      CmdSubcommands::getInstance()->init(
          *app_, cmdTree, emptyTree, emptySpecialCmds));

  // Verify that the command was added under the verb
  auto* showCmd = utils::getSubcommandIf(*app_, "show");
  ASSERT_NE(showCmd, nullptr);
  EXPECT_NE(utils::getSubcommandIf(*showCmd, "testObject"), nullptr);
}

TEST_F(CmdSubCommandsTest, InitWithNestedCommands) {
  CommandTree cmdTree = {
      RootCommand(
          "show",
          "parent",
          "Parent command help",
          {Command(
              "child",
              "Child command help",
              []() {},
              []() {
                return utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
              })}),
  };
  CommandTree emptyTree;
  std::vector<Command> emptySpecialCmds;

  EXPECT_NO_THROW(
      CmdSubcommands::getInstance()->init(
          *app_, cmdTree, emptyTree, emptySpecialCmds));

  // Verify nested command structure
  auto* showCmd = utils::getSubcommandIf(*app_, "show");
  ASSERT_NE(showCmd, nullptr);
  auto* parentCmd = utils::getSubcommandIf(*showCmd, "parent");
  ASSERT_NE(parentCmd, nullptr);
  EXPECT_NE(utils::getSubcommandIf(*parentCmd, "child"), nullptr);
}

TEST_F(CmdSubCommandsTest, ThrowsOnUnsupportedVerb) {
  CommandTree cmdTree = {
      RootCommand(
          "unsupported_verb",
          "testObject",
          "Test object help",
          []() {},
          []() { return utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE; }),
  };
  CommandTree emptyTree;
  std::vector<Command> emptySpecialCmds;

  EXPECT_THROW(
      CmdSubcommands::getInstance()->init(
          *app_, cmdTree, emptyTree, emptySpecialCmds),
      std::runtime_error);
}

TEST_F(CmdSubCommandsTest, ThrowsOnDuplicateCommand) {
  // Create a command tree with a duplicate command at the same level
  CommandTree cmdTree = {
      RootCommand(
          "show",
          "duplicate",
          "First duplicate help",
          []() {},
          []() { return utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE; }),
  };

  // Second tree with the same command name under the same verb
  CommandTree additionalTree = {
      RootCommand(
          "show",
          "duplicate",
          "Second duplicate help",
          []() {},
          []() { return utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE; }),
  };

  std::vector<Command> emptySpecialCmds;

  // Should throw because "duplicate" command already exists under "show"
  EXPECT_THROW(
      CmdSubcommands::getInstance()->init(
          *app_, cmdTree, additionalTree, emptySpecialCmds),
      std::runtime_error);
}

TEST_F(CmdSubCommandsTest, ThrowsOnDuplicateNestedCommand) {
  // Create a command tree with duplicate nested commands
  CommandTree cmdTree = {
      RootCommand(
          "show",
          "parent",
          "Parent command help",
          {Command(
               "child",
               "First child help",
               []() {},
               []() {
                 return utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
               }),
           Command(
               "child",
               "Duplicate child help",
               []() {},
               []() {
                 return utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
               })}),
  };
  CommandTree emptyTree;
  std::vector<Command> emptySpecialCmds;

  // This test verifies our duplicate detection code throws with the correct
  // message. If this test fails with a CLI11 error message instead, it means
  // the 'throw' keyword is missing in addCommandBranch().
  try {
    CmdSubcommands::getInstance()->init(
        *app_, cmdTree, emptyTree, emptySpecialCmds);
    FAIL() << "Expected std::runtime_error to be thrown";
  } catch (const std::runtime_error& e) {
    // Verify the exception message is from our code, not CLI11
    EXPECT_THAT(
        std::string(e.what()),
        ::testing::HasSubstr("Command `child` already exists"));
  }
}

TEST_F(CmdSubCommandsTest, InitWithSpecialCommands) {
  // This test covers lines 285-288 in CmdSubcommands.cpp:init()
  // Special commands are added directly to the root CLI app, bypassing the
  // verb structure (e.g., "show", "clear"). This allows commands like "help"
  // that don't follow the typical "<verb> <object>" pattern.
  //
  // Regular commands: app -> verb -> object (e.g., "fboss2 show port")
  // Special commands: app -> command directly (e.g., "fboss2 help")
  //
  // See kSpecialCommands() in CmdList.cpp for actual special commands.
  CommandTree emptyTree;
  std::vector<Command> specialCmds = {
      Command(
          "special",
          "Special command help",
          []() {},
          []() { return utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE; }),
  };

  EXPECT_NO_THROW(
      CmdSubcommands::getInstance()->init(
          *app_, emptyTree, emptyTree, specialCmds));

  // Verify that special command was added directly to app (not under a verb)
  EXPECT_NE(utils::getSubcommandIf(*app_, "special"), nullptr);
}

TEST_F(CmdSubCommandsTest, InitWithMultipleVerbs) {
  CommandTree cmdTree = {
      RootCommand(
          "show",
          "object1",
          "Show object1 help",
          []() {},
          []() { return utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE; }),
      RootCommand(
          "clear",
          "object2",
          "Clear object2 help",
          []() {},
          []() { return utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE; }),
  };
  CommandTree emptyTree;
  std::vector<Command> emptySpecialCmds;

  EXPECT_NO_THROW(
      CmdSubcommands::getInstance()->init(
          *app_, cmdTree, emptyTree, emptySpecialCmds));

  // Verify commands under different verbs
  auto* showCmd = utils::getSubcommandIf(*app_, "show");
  ASSERT_NE(showCmd, nullptr);
  EXPECT_NE(utils::getSubcommandIf(*showCmd, "object1"), nullptr);

  auto* clearCmd = utils::getSubcommandIf(*app_, "clear");
  ASSERT_NE(clearCmd, nullptr);
  EXPECT_NE(utils::getSubcommandIf(*clearCmd, "object2"), nullptr);
}

} // namespace
} // namespace facebook::fboss
