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
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/fabric/CmdShowFabric.h"
#include "fboss/cli/fboss2/commands/show/fabric/inputbalance/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/lib/inputbalance/InputBalanceUtil.h"

namespace facebook::fboss {

struct CmdShowFabricInputBalanceTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowFabric;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_SWITCH_NAME_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = cli::ShowFabricInputBalanceModel;
};
class CmdShowFabricInputBalance : public CmdHandler<
                                      CmdShowFabricInputBalance,
                                      CmdShowFabricInputBalanceTraits> {
 public:
  using ObjectArgType = CmdShowFabricInputBalanceTraits::ObjectArgType;
  using RetType = CmdShowFabricInputBalanceTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedSwitchNames);

  RetType createModel(
      const std::vector<utility::InputBalanceResult>& inputBalanceResults);

  void printOutput(const RetType& model, std::ostream& out = std::cout);

 private:
  // Returns unordered_map<neighborSwitch, unordered_map<destinationSwitch,
  // std::vector<neighboringPorts>>>
  std::unordered_map<
      std::string,
      std::unordered_map<std::string, std::vector<std::string>>>
  getNeighborReachability(
      const std::vector<std::string>& devicesToQueryInputCapacity,
      const std::unordered_map<
          std::string,
          std::unordered_map<std::string, std::string>>& neighborName2Ports,
      const std::vector<std::string>& dstSwitchNames);
};
} // namespace facebook::fboss
