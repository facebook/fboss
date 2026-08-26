/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <re2/re2.h>

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/bgp/CmdShowUtils.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "neteng/fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "neteng/fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"

namespace facebook::fboss {

using facebook::neteng::fboss::bgp::thrift::TBgpSession;

struct CmdShowBgpNeighborsByNameTraits : public ReadCommandTraits {
  using ParentCmd = void;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IP_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = std::vector<TBgpSession>;
};

class CmdShowBgpNeighborsByName : public CmdHandler<
                                      CmdShowBgpNeighborsByName,
                                      CmdShowBgpNeighborsByNameTraits> {
 public:
  using RetType = CmdShowBgpNeighborsByNameTraits::RetType;

  static std::vector<TBgpSession> filterNeighborsByNameRegex(
      const std::vector<TBgpSession>& allSessions,
      const std::string& pattern);

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& regexPatterns);

  void printOutput(const RetType& neighbors, std::ostream& out = std::cout);
};
} // namespace facebook::fboss
