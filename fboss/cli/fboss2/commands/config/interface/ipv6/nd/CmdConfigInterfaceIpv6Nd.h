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
#include <utility>
#include <vector>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/interface/ipv6/CmdConfigInterfaceIpv6.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

/*
 * NdpConfigAttrs captures the attribute-value pairs for the
 * `config interface <intf> ipv6 nd` command.
 *
 * Supported attributes (valueful):
 *   ra-interval <seconds>          - Router Advertisement interval (>= 0; 0 =
 * disabled) ra-lifetime <seconds>          - Router lifetime in RA messages (>=
 * 0) hop-limit <0-255>              - IPv6 hop limit advertised in RA
 *   prefix-valid-lifetime <sec>    - Prefix valid lifetime
 *   prefix-preferred-lifetime <sec>- Prefix preferred lifetime
 *   ra-address <ip>                - Source IP used in RA
 *
 * Supported attributes (valueless):
 *   managed-config-flag            - Set M-bit in RA (DHCPv6 addresses)
 *   other-config-flag              - Set O-bit in RA (DHCPv6 other info)
 */
class NdpConfigAttrs : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ NdpConfigAttrs(std::vector<std::string> v);

  const std::vector<std::pair<std::string, std::string>>& getAttributes()
      const {
    return attributes_;
  }

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NDP_CONFIG_ATTRS;

 private:
  std::vector<std::pair<std::string, std::string>> attributes_;
};

struct CmdConfigInterfaceIpv6NdTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigInterfaceIpv6;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NDP_CONFIG_ATTRS;
  using ObjectArgType = NdpConfigAttrs;
  using RetType = std::string;
};

class CmdConfigInterfaceIpv6Nd : public CmdHandler<
                                     CmdConfigInterfaceIpv6Nd,
                                     CmdConfigInterfaceIpv6NdTraits> {
 public:
  using ObjectArgType = CmdConfigInterfaceIpv6NdTraits::ObjectArgType;
  using RetType = CmdConfigInterfaceIpv6NdTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::InterfaceList& interfaces,
      const ObjectArgType& ndpAttrs);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
