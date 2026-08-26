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

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/bgp/neighbors_by_name/received/BgpNeighborsByNameReceivedRejected.h"

namespace facebook::fboss {

struct BgpNeighborsByNameReceivedRejectedCrfTraits : public ReadCommandTraits {
  using ParentCmd = BgpNeighborsByNameReceivedRejected;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::map<std::string, std::vector<std::string>>;
};

class BgpNeighborsByNameReceivedRejectedCrf
    : public CmdHandler<
          BgpNeighborsByNameReceivedRejectedCrf,
          BgpNeighborsByNameReceivedRejectedCrfTraits> {
 public:
  using RetType = BgpNeighborsByNameReceivedRejectedCrfTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& regexPatterns,
      const std::vector<std::string>& prefixes);

  void printOutput(const RetType& result, std::ostream& out = std::cout);
};
} // namespace facebook::fboss
