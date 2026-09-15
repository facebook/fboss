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
#include "fboss/cli/fboss2/commands/config/interface/InterfaceAttrArgsBase.h"

namespace facebook::fboss {

/*
 * InterfaceDeleteConfig captures interface/port names plus the attributes to
 * delete (reset to default or remove by value) for the `delete interface`
 * command.
 *
 * Usage: delete interface <intf-list> [<attr> [<value>] ...]
 *
 * Each entry of <intf-list> is a port name (eth1/1/1), an interface name, or
 * a bare interface ID — generated configs leave Interface.name unset, so the
 * ID is the only handle for those. With no attribute the whole object is
 * deleted: a port name deletes the port and its dependents, an interface
 * deletes the L3 interface itself.
 *
 * Valueless attributes (reset to default):
 *   loopback-mode, lookup-class, lldp-expected-value, lldp-expected-chassis,
 *   lldp-expected-ttl, lldp-expected-port-desc, lldp-expected-system-name,
 *   lldp-expected-system-desc
 *
 * Valued attributes (remove specific entry):
 *   ip-address <cidr>    — remove an IPv4 address (e.g. 10.0.0.1/24)
 *   ipv6-address <cidr>  — remove an IPv6 address (e.g. 2001:db8::1/64)
 */
class InterfaceDeleteConfig : public InterfaceAttrArgsBase {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ InterfaceDeleteConfig(const std::vector<std::string>& v);
};

struct CmdDeleteInterfaceTraits : public WriteCommandTraits {
  using ParentCmd = void;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
        "interface_delete_config",
        args,
        "<port-name|intf-name|intf-id>... [loopback-mode|lookup-class|lldp-expected-*|ip-address <cidr>|ipv6-address <cidr>]");
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
      const HostInfo& hostInfo,
      const ObjectArgType& deleteConfig);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
