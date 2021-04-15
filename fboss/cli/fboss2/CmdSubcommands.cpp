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

#include "fboss/cli/fboss2/CmdList.h"

#include <folly/Singleton.h>

namespace {
struct singleton_tag_type {};

const std::map<std::string, std::string>& kSupportedVerbs() {
  static const std::map<std::string, std::string> supportedVerbs = {
      {"show", "Show object info"},
  };

  return supportedVerbs;
};

} // namespace

using facebook::fboss::CmdSubcommands;
static folly::Singleton<CmdSubcommands, singleton_tag_type>
    cmdSubcommands{};
std::shared_ptr<CmdSubcommands> CmdSubcommands::getInstance() {
  return cmdSubcommands.try_get();
}

namespace facebook::fboss {

void CmdSubcommands::init(CLI::App& app) {
  for (const auto& [verb, helpMsg] : kSupportedVerbs()) {
    app.add_subcommand(verb, helpMsg);
  }

  for (const auto& [verb, object, helpMsg] : kListOfCommands()) {
    auto* verbSubCmd = app.get_subcommand_if(verb);

    // TODO explore moving this check to a compile time check
    if (!verbSubCmd) {
      throw std::runtime_error("unsupported verb " + verb);
    }

    verbSubCmd->add_subcommand(object, helpMsg);
  }
}

} // namespace facebook::fboss
