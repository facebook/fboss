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
#include "fboss/cli/fboss2/commands/show/fabric/inputbalance/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/lib/inputbalance/InputBalanceUtil.h"

namespace facebook::fboss {

struct CmdShowFabricInputBalanceTraits : public BaseCommandTraits {
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
      const ObjectArgType& queriedSwitchNames) {
    if (queriedSwitchNames.empty()) {
      throw std::runtime_error(
          "Please provide the list of switch name(s) to query reachability.");
    }
    std::vector<std::string> dstSwitchName;
    dstSwitchName.reserve(queriedSwitchNames.size());
    for (const auto& queriedSwitchName : queriedSwitchNames) {
      dstSwitchName.push_back(utils::removeFbDomains(queriedSwitchName));
    }

    auto fbossCtrlClient =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);

    std::map<int64_t, cfg::SwitchInfo> switchIdToSwitchInfo;
    fbossCtrlClient->sync_getSwitchIdToSwitchInfo(switchIdToSwitchInfo);
    if (std::none_of(
            switchIdToSwitchInfo.cbegin(),
            switchIdToSwitchInfo.cend(),
            [](const auto& entry) {
              return entry.second.switchType() == cfg::SwitchType::FABRIC;
            })) {
      throw std::runtime_error(
          "No Fabric device found in SwitchIdToSwitchInfo. "
          "Input Balance only applicable to FABRIC devices.");
    }

    std::map<int64_t, cfg::DsfNode> dsfNodeMap;
    fbossCtrlClient->sync_getDsfNodes(dsfNodeMap);
    bool isDualStage = utility::isDualStage(dsfNodeMap);

    // TODO(zecheng): Support for dual stage. Starting with single stage for
    // now.
    CHECK(!isDualStage);

    std::vector<int64_t> fabricSwitchIDs;
    for (const auto& [switchId, switchInfo] : switchIdToSwitchInfo) {
      if (switchInfo.switchType() == cfg::SwitchType::FABRIC) {
        fabricSwitchIDs.push_back(switchId);
      }
    }

    auto deviceToQueryInputCapacity =
        utility::deviceToQueryInputCapacity(fabricSwitchIDs, dsfNodeMap);

    if (deviceToQueryInputCapacity.empty()) {
      throw std::runtime_error(
          "Failed to find devices to query input capacity");
    }

    return createModel();
  }

  RetType createModel() {
    // TODO(zecheng): Create model for input balance
    return {};
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    // TODO(zecheng): Print output of input balance
  }
};
} // namespace facebook::fboss
