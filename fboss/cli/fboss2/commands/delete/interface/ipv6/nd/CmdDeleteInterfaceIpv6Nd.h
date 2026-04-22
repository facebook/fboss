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
#include "fboss/cli/fboss2/commands/delete/interface/ipv6/CmdDeleteInterfaceIpv6.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

/*
 * NdpDeleteAttrs captures the list of NDP attribute names to reset to defaults
 * for the `delete interface <intf> ipv6 nd` command.
 *
 * Supported attributes (all valueless — only names are provided):
 *   ra-interval               - Reset Router Advertisement interval to 0
 *   ra-lifetime               - Reset Router lifetime to 1800
 *   hop-limit                 - Reset IPv6 hop limit to 255
 *   prefix-valid-lifetime     - Reset prefix valid lifetime to 2592000
 *   prefix-preferred-lifetime - Reset prefix preferred lifetime to 604800
 *   managed-config-flag       - Reset M-bit to false
 *   other-config-flag         - Reset O-bit to false
 *   ra-address                - Clear the router address (optional reset)
 */
class NdpDeleteAttrs : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ NdpDeleteAttrs(const std::vector<std::string>& v);

  const std::vector<std::string>& getAttributes() const {
    return attributes_;
  }

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NDP_DELETE_ATTRS;

 private:
  std::vector<std::string> attributes_;
};

struct CmdDeleteInterfaceIpv6NdTraits : public WriteCommandTraits {
  using ParentCmd = CmdDeleteInterfaceIpv6;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NDP_DELETE_ATTRS;
  using ObjectArgType = NdpDeleteAttrs;
  using RetType = std::string;
};

class CmdDeleteInterfaceIpv6Nd : public CmdHandler<
                                     CmdDeleteInterfaceIpv6Nd,
                                     CmdDeleteInterfaceIpv6NdTraits> {
 public:
  using ObjectArgType = CmdDeleteInterfaceIpv6NdTraits::ObjectArgType;
  using RetType = CmdDeleteInterfaceIpv6NdTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::InterfaceList& interfaces,
      const ObjectArgType& ndpAttrs);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
