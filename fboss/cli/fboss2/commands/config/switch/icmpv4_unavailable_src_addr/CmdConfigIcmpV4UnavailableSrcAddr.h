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

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/switch/CmdConfigSwitch.h"

namespace facebook::fboss {

class IcmpV4UnavailableSrcAddrArg
    : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */
  IcmpV4UnavailableSrcAddrArg( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  const std::string& getAddress() const {
    return data_[0];
  }
};

struct CmdConfigIcmpV4UnavailableSrcAddrTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigSwitch;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
           "ipv4-address",
           args,
           "IPv4 source address for ICMP when no address is configured")
        ->required()
        ->expected(1);
  }
  using ObjectArgType = IcmpV4UnavailableSrcAddrArg;
  using RetType = std::string;
};

class CmdConfigIcmpV4UnavailableSrcAddr
    : public CmdHandler<
          CmdConfigIcmpV4UnavailableSrcAddr,
          CmdConfigIcmpV4UnavailableSrcAddrTraits> {
 public:
  using ObjectArgType = CmdConfigIcmpV4UnavailableSrcAddrTraits::ObjectArgType;
  using RetType = CmdConfigIcmpV4UnavailableSrcAddrTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& srcAddr);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
