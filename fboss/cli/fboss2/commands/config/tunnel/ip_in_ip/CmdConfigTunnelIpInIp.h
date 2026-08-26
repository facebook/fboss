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
#include "fboss/cli/fboss2/commands/config/tunnel/CmdConfigTunnel.h"

namespace facebook::fboss {

/*
 * Branch node for 'config tunnel ip-in-ip'. The actual work is done by the
 * 'encap' and 'decap' subcommands; this handler only exists so the command
 * tree increments depth correctly and prints a helpful message when invoked
 * without a direction.
 */
struct CmdConfigTunnelIpInIpTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigTunnel;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = utils::NoneArgType;
  using RetType = std::string;
};

class CmdConfigTunnelIpInIp
    : public CmdHandler<CmdConfigTunnelIpInIp, CmdConfigTunnelIpInIpTraits> {
 public:
  using ObjectArgType = CmdConfigTunnelIpInIpTraits::ObjectArgType;
  using RetType = CmdConfigTunnelIpInIpTraits::RetType;

  RetType queryClient(const HostInfo& /* hostInfo */) {
    throw std::runtime_error(
        "Incomplete command, please use 'encap' or 'decap' subcommand");
  }

  void printOutput(const RetType& /* model */) {}
};

} // namespace facebook::fboss
