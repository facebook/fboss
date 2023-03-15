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
using GetValidFilterFn = std::function<ValidFilterMapType()>;

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
      const std::vector<Command>& subcommands = {},
      const std::vector<utils::LocalOption>& localOptions = {})
      : name{name},
        argType{argType},
        help{help},
        handler{handler},
        subcommands{subcommands},
        localOptions{localOptions} {}

  // Some commands don't have handlers and only have more subcommands
  Command(
      const std::string& name,
      const std::string& help,
      const std::vector<Command>& subcommands)
      : name{name},
        argType{utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE},
        help{help},
        subcommands{subcommands} {}

  Command(
      const std::string& name,
      const utils::ObjectArgTypeId argType,
      const std::string& help,
      const CommandHandlerFn& handler,
      const GetValidFilterFn& validFilterGetter,
      const std::vector<Command>& subcommands = {},
      const std::vector<utils::LocalOption>& localOptions = {})
      : name{name},
        argType{argType},
        help{help},
        handler{handler},
        validFilterHandler{validFilterGetter},
        subcommands{subcommands},
        localOptions{localOptions} {}

  const std::string name;
  const utils::ObjectArgTypeId argType;
  const std::string help;
  const std::optional<CommandHandlerFn> handler;
  const std::optional<GetValidFilterFn> validFilterHandler;
  const std::vector<Command> subcommands;
  const std::vector<utils::LocalOption> localOptions;
};

struct RootCommand : public Command {
  RootCommand(
      const std::string& verb,
      const std::string& object,
      const utils::ObjectArgTypeId argType,
      const std::string& help,
      const CommandHandlerFn& handler,
      const std::vector<Command>& subcommands = {},
      const std::vector<utils::LocalOption>& localOptions = {})
      : Command(object, argType, help, handler, subcommands, localOptions),
        verb{verb} {}

  RootCommand(
      const std::string& verb,
      const std::string& object,
      const utils::ObjectArgTypeId argType,
      const std::string& help,
      const CommandHandlerFn& handler,
      const GetValidFilterFn& validFilterGetter,
      const std::vector<Command>& subcommands = {},
      const std::vector<utils::LocalOption>& localOptions = {})
      : Command(
            object,
            argType,
            help,
            handler,
            validFilterGetter,
            subcommands,
            localOptions),
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

  HelpInfo(
      const CmdVerb& verb,
      const CmdHelpMsg& helpMsg,
      const ValidFilterMapType& validFilterMap)
      : verb(verb), helpMsg(helpMsg), validFilterMap(validFilterMap) {}

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

template <typename T>
ValidFilterMapType getValidFilterHandler() {
  return T().getValidFilters();
}

void helpHandler();

} // namespace facebook::fboss
