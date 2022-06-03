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

#include <functional>
#include <string>
#include <tuple>
#include <vector>

#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

using CommandHandlerFn = std::function<void()>;

using CmdVerb = std::string;
using CmdObject = std::string;
using CmdSubCmd = std::string;
using CmdHelpMsg = std::string;

struct Command {
  Command(
      const std::string& name,
      const utils::ObjectArgTypeId argType,
      const std::string& help,
      const CommandHandlerFn& handler,
      const std::vector<Command>& subcommands = {})
      : name{name},
        argType{argType},
        help{help},
        handler{handler},
        subcommands{subcommands} {}

  // Some commands don't have handlers and only have more subcommands
  Command(
      const std::string& name,
      const std::string& help,
      const std::vector<Command>& subcommands)
      : name{name},
        argType{utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE},
        help{help},
        subcommands{subcommands} {}

  const std::string name;
  const utils::ObjectArgTypeId argType;
  const std::string help;
  const std::optional<CommandHandlerFn> handler;
  const std::vector<Command> subcommands;
};

struct RootCommand : public Command {
  RootCommand(
      const std::string& verb,
      const std::string& object,
      const utils::ObjectArgTypeId argType,
      const std::string& help,
      const CommandHandlerFn& handler,
      const std::vector<Command>& subcommands = {})
      : Command(object, argType, help, handler, subcommands), verb{verb} {}

  RootCommand(
      const std::string& verb,
      const std::string& name,
      const std::string& help,
      const std::vector<Command>& subcommands)
      : Command(name, help, subcommands), verb{verb} {}

  std::string verb;
};

using CommandTree = std::vector<RootCommand>;

//(verb, verb_help) -> vector of subcommands
using RevHelpForObj =
    std::map<std::pair<CmdVerb, CmdHelpMsg>, std::vector<Command>>;

// obj -> revHelpForObj.
using ReverseHelpTree = std::map<std::string, RevHelpForObj>;

const CommandTree& kCommandTree();
const CommandTree& kAdditionalCommandTree();
ReverseHelpTree kRevHelpTree(const CommandTree& cmdTree);

const std::vector<Command>& kSpecialCommands();

template <typename T>
void commandHandler() {
  T().run();
}

template <typename T>
void helpHandler() {
  auto cmdTree = kCommandTree();
  auto revHelpTree = kRevHelpTree(cmdTree);
  T(revHelpTree).run();
}

} // namespace facebook::fboss
