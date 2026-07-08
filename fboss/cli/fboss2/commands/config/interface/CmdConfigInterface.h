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

#include <set>
#include <string>
#include <vector>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/interface/InterfaceAttrArgsBase.h"
#include "fboss/cli/fboss2/commands/config/interface/ProfileValidation.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

class PlatformMapping;

/*
 * InterfacesConfig captures both port/interface names and optional
 * attribute key-value pairs from the CLI.
 *
 * Usage: config interface <port-list> [<attr> [<value>] ...]
 *
 * The first tokens (until a known attribute name is encountered) are
 * treated as port/interface names. The remaining tokens are parsed
 * as attribute-value pairs (see InterfaceAttrArgsBase).
 */
class InterfacesConfig : public InterfaceAttrArgsBase {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ InterfacesConfig(const std::vector<std::string>& v);
};

struct CmdConfigInterfaceTraits : public WriteCommandTraits {
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
        "interface_config",
        args,
        "<port-list> [<attr> <value> ...] where <attr> is one "
        "of: description, mtu, ip-address, ipv6-address, ...");
  }
  using ObjectArgType = InterfacesConfig;
  using RetType = std::string;
};

class CmdConfigInterface
    : public CmdHandler<CmdConfigInterface, CmdConfigInterfaceTraits> {
 public:
  using ObjectArgType = CmdConfigInterfaceTraits::ObjectArgType;
  using RetType = CmdConfigInterfaceTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& interfaceConfig);

  void printOutput(const RetType& logMsg);
};

// Testable core of the profile-change flow. The `profile` attribute both
// narrows/removes existing ports and creates absent-but-valid INTERFACE_PORTs,
// so this is the single "apply profile" step. All validation (per-port support,
// controlling-port subsumption, and creatability of absent ports) runs before
// any mutation; then existing ports are adjusted and subsumed ports removed
// before absent ports are created. Throws std::invalid_argument with a
// user-facing message on any validation failure (leaving `swConfig`
// unchanged). Declared here for unit testing.
std::string applyProfileImpl(
    ProfileValidator& validator,
    cfg::SwitchConfig& swConfig,
    const utils::InterfaceList& interfaces,
    const std::string& value);

} // namespace facebook::fboss
