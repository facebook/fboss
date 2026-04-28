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

#include <map>
#include <string>
#include <string_view>
#include <vector>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/tunnel/CmdConfigTunnel.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

namespace facebook::fboss {

inline constexpr std::string_view kTunnelModeUniform = "uniform";
inline constexpr std::string_view kTunnelModePipe = "pipe";

inline constexpr std::string_view kTermTypeP2P = "p2p";
inline constexpr std::string_view kTermTypeP2MP = "p2mp";
inline constexpr std::string_view kTermTypeMP2P = "mp2p";
inline constexpr std::string_view kTermTypeMP2MP = "mp2mp";

/*
 * TunnelIpInIpConfig captures the tunnel ID and optional (attr, value) pairs.
 *
 * Usage: config tunnel ip-in-ip <tunnel-id> [<attr> <value> ...]
 *
 * Supported attributes:
 *   destination  <ipv6> [mask <prefix-len>]   IPv6 addresses only
 *   source       <ipv6> [mask <prefix-len>]   IPv6 addresses only
 *   ttl-mode     uniform|pipe
 *   dscp-mode    uniform|pipe
 *   ecn-mode     uniform|pipe
 *   termination-type  p2p|p2mp|mp2p|mp2mp
 *   underlay-intf-id  <int>
 *
 * Attribute names and enum values are matched case-insensitively.
 * Prefix lengths for mask must be integers in [0, 128].
 */
class TunnelIpInIpConfig : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ TunnelIpInIpConfig(std::vector<std::string> v);

  const std::string& getTunnelId() const {
    return tunnelId_;
  }

  // Map of attribute name to value
  const std::map<std::string, std::string>& getAttrs() const {
    return attrs_;
  }

  bool hasAttrs() const {
    return !attrs_.empty();
  }

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_TUNNEL_IP_IN_IP_ID;

 private:
  static bool isKnownAttribute(const std::string& s);

  std::string tunnelId_;
  std::map<std::string, std::string> attrs_;
};

struct CmdConfigTunnelIpInIpTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigTunnel;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_TUNNEL_IP_IN_IP_ID;
  using ObjectArgType = TunnelIpInIpConfig;
  using RetType = std::string;
};

class CmdConfigTunnelIpInIp
    : public CmdHandler<CmdConfigTunnelIpInIp, CmdConfigTunnelIpInIpTraits> {
 public:
  using ObjectArgType = CmdConfigTunnelIpInIpTraits::ObjectArgType;
  using RetType = CmdConfigTunnelIpInIpTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& tunnelConfig);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
