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

#include <string>
#include <vector>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/interface/InterfaceAttrUtils.h"

namespace facebook::fboss {

/*
 * InterfaceDeleteConfig captures interface/port names plus the attributes to
 * delete (reset to default) for the `delete interface` command.
 *
 * Usage: delete interface <port-list> [<attr> ...]
 *
 * Tokens before the first known attribute name are interface/port names; the
 * rest are attribute names (valueless — delete always resets to default).
 *
 * Supported delete attributes (reset to default):
 *   loopback-mode, lldp-expected-value, lldp-expected-chassis,
 *   lldp-expected-ttl, lldp-expected-port-desc, lldp-expected-system-name,
 *   lldp-expected-system-desc
 */
class InterfaceDeleteConfig : public InterfaceAttrArgsBase {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ InterfaceDeleteConfig(const std::vector<std::string>& v);

 private:
  // Check if a string is a known delete attribute name (valueful or valueless)
  static bool isKnownAttribute(const std::string& s);
  // Check if a known attribute takes no value token
  static bool isValuelessAttribute(const std::string& s);
};

struct CmdDeleteInterfaceTraits : public WriteCommandTraits {
  using ParentCmd = void;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
        "interface_delete_config",
        args,
        "<port-list> [loopback-mode|lldp-expected-*]");
  }
  using ObjectArgType = InterfaceDeleteConfig;
  using RetType = std::string;
};

class CmdDeleteInterface
    : public CmdHandler<CmdDeleteInterface, CmdDeleteInterfaceTraits> {
 public:
  using ObjectArgType = CmdDeleteInterfaceTraits::ObjectArgType;
  using RetType = CmdDeleteInterfaceTraits::RetType;

  RetType queryClient(
      const HostInfo& /* hostInfo */,
      const ObjectArgType& /* interfaceConfig */) {
    throw std::runtime_error(
        "Incomplete command, please use one of the subcommands (e.g. ipv6 ndp)");
  }

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
