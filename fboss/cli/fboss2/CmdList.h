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

#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/utils/CmdCommonUtils.h"

namespace facebook::fboss {

using ValidFilterMapType = std::unordered_map<
    std::string_view,
    std::shared_ptr<CmdGlobalOptions::BaseTypeVerifier>>;
using CommandHandlerFn = std::function<void()>;
using ValidFilterHandlerFn = std::function<ValidFilterMapType()>;
using ArgTypeHandlerFn = std::function<utils::ObjectArgTypeId()>;
using LocalOptionsHandlerFn = std::function<std::vector<utils::LocalOption>()>;

using CmdVerb = std::string;
using CmdObject = std::string;
using CmdSubCmd = std::string;
using CmdHelpMsg = std::string;

struct Command {
  Command(
      const std::string& name,
      const std::string& help,
      const CommandHandlerFn& commandHandler,
      const ArgTypeHandlerFn& argTypeHandler,
      const std::vector<Command>& subcommands = {})
      : name{name},
        help{help},
        commandHandler{commandHandler},
        argTypeHandler{argTypeHandler},
        subcommands{subcommands} {}

  Command(
      const std::string& name,
      const std::string& help,
      const CommandHandlerFn& commandHandler,
      const ValidFilterHandlerFn& validFilterHandler,
      const ArgTypeHandlerFn& argTypeHandler,
      const std::vector<Command>& subcommands = {})
      : name{name},
        help{help},
        commandHandler{commandHandler},
        validFilterHandler{validFilterHandler},
        argTypeHandler{argTypeHandler},
        subcommands{subcommands} {}

  Command(
      const std::string& name,
      const std::string& help,
      const CommandHandlerFn& commandHandler,
      const ArgTypeHandlerFn& argTypeHandler,
      const LocalOptionsHandlerFn& localOptionsHandler,
      const std::vector<Command>& subcommands = {})
      : name{name},
        help{help},
        commandHandler{commandHandler},
        argTypeHandler{argTypeHandler},
        localOptionsHandler{localOptionsHandler},
        subcommands{subcommands} {}

  Command(
      const std::string& name,
      const std::string& help,
      const CommandHandlerFn& commandHandler,
      const ValidFilterHandlerFn& validFilterHandler,
      const ArgTypeHandlerFn& argTypeHandler,
      const LocalOptionsHandlerFn& localOptionsHandler,
      const std::vector<Command>& subcommands = {})
      : name{name},
        help{help},
        commandHandler{commandHandler},
        validFilterHandler{validFilterHandler},
        argTypeHandler{argTypeHandler},
        localOptionsHandler{localOptionsHandler},
        subcommands{subcommands} {}

  // Some commands don't have handlers and only have more subcommands
  Command(
      const std::string& name,
      const std::string& help,
      const std::vector<Command>& subcommands)
      : name{name}, help{help}, subcommands{subcommands} {}

  const std::string name;
  const std::string help;
  const std::optional<CommandHandlerFn> commandHandler;
  const std::optional<ValidFilterHandlerFn> validFilterHandler;
  const std::optional<ArgTypeHandlerFn> argTypeHandler;
  const std::optional<LocalOptionsHandlerFn> localOptionsHandler;
  const std::vector<Command> subcommands;
};

struct RootCommand : public Command {
  RootCommand(
      const std::string& verb,
      const std::string& object,
      const std::string& help,
      const CommandHandlerFn& commandHandler,
      const ArgTypeHandlerFn& argTypeHandler,
      const std::vector<Command>& subcommands = {})
      : Command(object, help, commandHandler, argTypeHandler, subcommands),
        verb{verb} {}

  RootCommand(
      const std::string& verb,
      const std::string& object,
      const std::string& help,
      const CommandHandlerFn& commandHandler,
      const ValidFilterHandlerFn& validFilterHandler,
      const ArgTypeHandlerFn& argTypeHandler,
      const std::vector<Command>& subcommands = {})
      : Command(
            object,
            help,
            commandHandler,
            validFilterHandler,
            argTypeHandler,
            subcommands),
        verb{verb} {}

  RootCommand(
      const std::string& verb,
      const std::string& object,
      const std::string& help,
      const CommandHandlerFn& commandHandler,
      const ArgTypeHandlerFn& argTypeHandler,
      const LocalOptionsHandlerFn& localOptionsHandler,
      const std::vector<Command>& subcommands = {})
      : Command(
            object,
            help,
            commandHandler,
            argTypeHandler,
            localOptionsHandler,
            subcommands),
        verb{verb} {}

  RootCommand(
      const std::string& verb,
      const std::string& object,
      const std::string& help,
      const CommandHandlerFn& commandHandler,
      const ValidFilterHandlerFn& validFilterHandler,
      const ArgTypeHandlerFn& argTypeHandler,
      const LocalOptionsHandlerFn& localOptionsHandler,
      const std::vector<Command>& subcommands = {})
      : Command(
            object,
            help,
            commandHandler,
            validFilterHandler,
            argTypeHandler,
            localOptionsHandler,
            subcommands),
        verb{verb} {}

  RootCommand(
      const std::string& verb,
      const std::string& name,
      const std::string& help,
      const std::vector<Command>& subcommands)
      : Command(name, help, subcommands), verb{verb} {}

  std::string verb;
};

using CommandTree = std::vector<RootCommand>;

struct HelpInfo {
  CmdVerb verb;
  CmdHelpMsg helpMsg;
  ValidFilterMapType validFilterMap;
  std::vector<utils::LocalOption> localOptions;

  HelpInfo(
      const CmdVerb& verb,
      const CmdHelpMsg& helpMsg,
      const ValidFilterMapType& validFilterMap,
      const std::vector<utils::LocalOption>& localOptions)
      : verb(verb),
        helpMsg(helpMsg),
        validFilterMap(validFilterMap),
        localOptions(localOptions) {}

  bool operator==(const HelpInfo& info) const {
    return verb == info.verb && helpMsg == info.helpMsg;
  }
  bool operator<(const HelpInfo& info) const {
    return verb < info.verb;
  }
};

// HelpInfo -> vector of subcommands
using RevHelpForObj = std::map<HelpInfo, std::vector<Command>>;

// obj -> revHelpForObj.
using ReverseHelpTree = std::map<std::string, RevHelpForObj>;

const CommandTree& kCommandTree();
const CommandTree& kAdditionalCommandTree();

const std::vector<Command>& kSpecialCommands();

template <typename T>
void commandHandler() {
  T().run();
}

void helpCommandHandler();

template <typename T>
ValidFilterMapType validFilterHandler() {
  return T().getValidFilters();
}

template <typename T>
utils::ObjectArgTypeId argTypeHandler() {
  return T().ObjectArgTypeId;
}

utils::ObjectArgTypeId helpArgTypeHandler();

template <typename T>
std::vector<utils::LocalOption> localOptionsHandler() {
  return T().LocalOptions;
}

} // namespace facebook::fboss
