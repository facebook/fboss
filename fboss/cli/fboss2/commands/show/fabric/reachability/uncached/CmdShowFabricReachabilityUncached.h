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
#include "fboss/cli/fboss2/commands/show/fabric/reachability/CmdShowFabricReachability.h"

namespace facebook::fboss {

struct CmdShowFabricReachabilityUncachedTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowFabricReachability;
  using RetType = cli::ShowFabricReachabilityModel;
};
class CmdShowFabricReachabilityUncached
    : public CmdHandler<
          CmdShowFabricReachabilityUncached,
          CmdShowFabricReachabilityUncachedTraits>,
      public CmdShowFabricReachabilityTraits {
 public:
  using RetType = CmdShowFabricReachabilityUncachedTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedSwitchNames);

  RetType createModel(
      std::unordered_map<std::string, std::vector<std::string>>&
          reachabilityMatrix);
  void printOutput(const RetType& model, std::ostream& out = std::cout);
};
} // namespace facebook::fboss
