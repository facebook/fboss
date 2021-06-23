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

#include "fboss/cli/fboss2/CLI11/CLI.hpp"

#include "fboss/cli/fboss2/CmdList.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

class CmdSubcommands {
 public:
  CmdSubcommands() = default;
  ~CmdSubcommands() = default;
  CmdSubcommands(const CmdSubcommands& other) = delete;
  CmdSubcommands& operator=(const CmdSubcommands& other) = delete;

  // Static function for getting the CmdSubcommands folly::Singleton
  static std::shared_ptr<CmdSubcommands> getInstance();

  void init(CLI::App& app);

  std::vector<std::string> getIpv6Addrs() {
    return ipv6Addrs_;
  }

  std::vector<std::string> getPorts() {
    return ports_;
  }

 private:
  void initHelper(
      CLI::App& app,
      const std::vector<std::tuple<
          CmdVerb,
          CmdObject,
          utils::ObjectArgTypeId,
          CmdSubCmd,
          CmdHelpMsg,
          CommandHandlerFn>>& listOfCommands);

  std::vector<std::string> ipv6Addrs_;
  std::vector<std::string> ports_;
};

} // namespace facebook::fboss
