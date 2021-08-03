/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/CmdSubcommands.h"
#include "fboss/cli/fboss2/utils/CLIParserUtils.h"

#include <folly/Singleton.h>

namespace {
struct singleton_tag_type {};

const std::map<std::string, std::string>& kSupportedVerbs() {
  static const std::map<std::string, std::string> supportedVerbs = {
      {"show", "Show object info"},
      {"clear", "Clear object info"},
      {"create", "Create object"},
  };

  return supportedVerbs;
};

} // namespace

using facebook::fboss::CmdSubcommands;
static folly::Singleton<CmdSubcommands, singleton_tag_type> cmdSubcommands{};
std::shared_ptr<CmdSubcommands> CmdSubcommands::getInstance() {
  return cmdSubcommands.try_get();
}

namespace facebook::fboss {

void CmdSubcommands::initHelper(
    CLI::App& app,
    const std::vector<std::tuple<
        CmdVerb,
        CmdObject,
        utils::ObjectArgTypeId,
        CmdSubCmd,
        CmdHelpMsg,
        CommandHandlerFn>>& listOfCommands) {
  for (
      const auto& [verb, object, objectArgType, subCmd, helpMsg, commandHandlerFn] :
      listOfCommands) {
    auto* verbCmd = utils::getSubcommandIf(app, verb);

    // TODO explore moving this check to a compile time check
    if (!verbCmd) {
      throw std::runtime_error("unsupported verb " + verb);
    }

    auto* objectCmd = utils::getSubcommandIf(*verbCmd, object);
    if (!objectCmd) {
      objectCmd = verbCmd->add_subcommand(object, helpMsg);
      objectCmd->callback(commandHandlerFn);

      if (objectArgType ==
          utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IPV6_LIST) {
        objectCmd->add_option("ipv6Addrs", ipv6Addrs_, "IPv6 addr(s)");
      } else if (
          objectArgType ==
          utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST) {
        objectCmd->add_option("ports", ports_, "Port(s)");
      }
    }

    if (!subCmd.empty()) {
      auto* objectSubCmd = objectCmd->add_subcommand(subCmd, helpMsg);
      objectSubCmd->callback(commandHandlerFn);
    }
  }
}

void CmdSubcommands::init(CLI::App& app) {
  for (const auto& [verb, helpMsg] : kSupportedVerbs()) {
    app.add_subcommand(verb, helpMsg);
  }

  initHelper(app, kListOfCommands());
  initHelper(app, kListOfAdditionalCommands());
}

} // namespace facebook::fboss
