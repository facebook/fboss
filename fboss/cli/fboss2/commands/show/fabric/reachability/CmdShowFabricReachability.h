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
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/fabric/CmdShowFabric.h"
#include "fboss/cli/fboss2/commands/show/fabric/reachability/gen-cpp2/model_types.h"

namespace facebook::fboss {

struct CmdShowFabricReachabilityTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowFabric;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_SWITCH_NAME_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = cli::ShowFabricReachabilityModel;
};

class CmdShowFabricReachability : public CmdHandler<
                                      CmdShowFabricReachability,
                                      CmdShowFabricReachabilityTraits> {
 public:
  using ObjectArgType = CmdShowFabricReachabilityTraits::ObjectArgType;
  using RetType = CmdShowFabricReachabilityTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedSwitchNames);

  static RetType createModel(
      std::unordered_map<std::string, std::vector<std::string>>&
          reachabilityMatrix);

  static void printOutput(const RetType& model, std::ostream& out = std::cout);
};
} // namespace facebook::fboss
