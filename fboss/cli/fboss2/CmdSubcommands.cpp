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
#include "fboss/cli/fboss2/CmdArgsLists.h"
#include "fboss/cli/fboss2/CmdList.h"
#include "fboss/cli/fboss2/utils/CLIParserUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

#include <folly/Singleton.h>
#include <stdexcept>

namespace {
struct singleton_tag_type {};

const std::map<std::string, std::string>& kSupportedVerbs() {
  static const std::map<std::string, std::string> supportedVerbs = {
      {"show", "Show object info"},
      {"clear", "Clear object info"},
      {"create", "Create object"},
      {"set", "Set object"},
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

CLI::App*
CmdSubcommands::addCommand(CLI::App& app, const Command& cmd, int depth) {
  auto* subCmd = app.add_subcommand(cmd.name, cmd.help);
  if (auto& handler = cmd.handler) {
    subCmd->callback(*handler);

    auto& args = CmdArgsLists::getInstance()->refAt(depth);
    if (cmd.argType ==
        utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_COMMUNITY_LIST) {
      subCmd->add_option("communities", args, "BGP community(s)");
    } else if (
        cmd.argType == utils::ObjectArgTypeId::OBJECT_ARG_TYPE_DEBUG_LEVEL) {
      subCmd->add_option("level", args, "Debug level");
    } else if (
        cmd.argType == utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IPV6_LIST) {
      subCmd->add_option("ipv6Addrs", args, "IPv6 addr(s)");
    } else if (
        cmd.argType == utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IP_LIST) {
      subCmd->add_option("ipAddrs", args, "IPv4 or IPv6 addr(s)");
    } else if (
        cmd.argType == utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PEERID_LIST) {
      subCmd->add_option("peerID", args, "BGP remote peer ID (int)");
    } else if (
        cmd.argType == utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST) {
      subCmd->add_option("ports", args, "Port(s)");
    } else if (
        cmd.argType == utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_MESSAGE) {
      subCmd->add_option("msg", args, "Message");
    }
  }
  return subCmd;
}

void CmdSubcommands::addCommandBranch(
    CLI::App& app,
    const Command& cmd,
    int depth) {
  // Command should not already exists since we only traverse the tree once
  if (utils::getSubcommandIf(app, cmd.name)) {
    // TODO explore moving this check to a compile time check
    std::runtime_error("Command already exists, command tree must be invalid");
  }
  auto* subCmd = addCommand(app, cmd, depth);
  if (cmd.handler) {
    // Do not increase depth for pass through commands that don't have arguments
    depth++;
  }
  for (const auto& child : cmd.subcommands) {
    addCommandBranch(*subCmd, child, depth);
  }
}

void CmdSubcommands::initCommandTree(
    CLI::App& app,
    const CommandTree& cmdTree) {
  for (const auto& cmd : cmdTree) {
    auto& verb = cmd.verb;
    auto* verbCmd = utils::getSubcommandIf(app, verb);
    // TODO explore moving this check to a compile time check
    if (!verbCmd) {
      throw std::runtime_error("Unsupported verb " + verb);
    }

    addCommandBranch(*verbCmd, cmd);
  }
}

void CmdSubcommands::init(CLI::App& app) {
  for (const auto& [verb, helpMsg] : kSupportedVerbs()) {
    app.add_subcommand(verb, helpMsg);
  }

  initCommandTree(app, kCommandTree());
  initCommandTree(app, kAdditionalCommandTree());

  for (const auto& cmd : kSpecialCommands()) {
    addCommand(app, cmd, 0);
  }
}

} // namespace facebook::fboss
