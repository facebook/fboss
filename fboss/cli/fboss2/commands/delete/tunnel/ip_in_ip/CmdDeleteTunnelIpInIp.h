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
#include "fboss/cli/fboss2/commands/delete/tunnel/CmdDeleteTunnel.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

namespace facebook::fboss {

/*
 * TunnelIpInIpDeleteArgs captures the tunnel ID and an optional list of
 * attribute names to reset.
 *
 * Usage:
 *   delete tunnel ip-in-ip <tunnel-id>                  - delete entire tunnel
 *   delete tunnel ip-in-ip <tunnel-id> <attr> [<attr>]  - reset listed attrs
 *
 * Supported (resettable) attributes — only optional fields can be reset:
 *   source, ttl-mode, dscp-mode, ecn-mode, termination-type
 *
 * Required fields (destination, underlay-intf-id) cannot be reset
 * individually; delete the whole tunnel instead.
 */
class TunnelIpInIpDeleteArgs : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ TunnelIpInIpDeleteArgs(const std::vector<std::string>& v);

  const std::string& getTunnelId() const {
    return tunnelId_;
  }

  const std::vector<std::string>& getAttributes() const {
    return attributes_;
  }

  bool deleteEntireTunnel() const {
    return attributes_.empty();
  }

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_TUNNEL_IP_IN_IP_DELETE_ID;

 private:
  std::string tunnelId_;
  std::vector<std::string> attributes_;
};

struct CmdDeleteTunnelIpInIpTraits : public WriteCommandTraits {
  using ParentCmd = CmdDeleteTunnel;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_TUNNEL_IP_IN_IP_DELETE_ID;
  using ObjectArgType = TunnelIpInIpDeleteArgs;
  using RetType = std::string;
};

class CmdDeleteTunnelIpInIp
    : public CmdHandler<CmdDeleteTunnelIpInIp, CmdDeleteTunnelIpInIpTraits> {
 public:
  using ObjectArgType = CmdDeleteTunnelIpInIpTraits::ObjectArgType;
  using RetType = CmdDeleteTunnelIpInIpTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& deleteArgs);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
