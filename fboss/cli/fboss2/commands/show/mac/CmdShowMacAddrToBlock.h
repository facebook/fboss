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

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <fboss/cli/fboss2/utils/CmdUtils.h>
#include <folly/String.h>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/mac/gen-cpp2/model_types.h"

namespace facebook::fboss {

struct CmdShowMacAddrToBlockTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = utils::NoneArgType;
  using RetType = cli::ShowMacAddrToBlockModel;
};

class CmdShowMacAddrToBlock
    : public CmdHandler<CmdShowMacAddrToBlock, CmdShowMacAddrToBlockTraits> {
 public:
  RetType queryClient(const HostInfo& hostInfo) {
    std::vector<cfg::MacAndVlan> entries;
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    client->sync_getMacAddrsToBlock(entries);
    return createModel(entries);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    constexpr auto fmtString = "{:<19}{:<19}\n";

    out << fmt::format(fmtString, "VLAN", "MAC Address");

    for (const auto& entry : model.get_macAndVlanEntries()) {
      out << fmt::format(fmtString, entry.get_vlanID(), entry.get_macAddress());
    }
    out << std::endl;
  }

  RetType createModel(std::vector<cfg::MacAndVlan>& macAndVlanEntries) {
    RetType model;

    for (const auto& entry : macAndVlanEntries) {
      cli::MacAndVlan macAndVlanDetails;

      macAndVlanDetails.vlanID() = entry.get_vlanID();
      macAndVlanDetails.macAddress() = entry.get_macAddress();

      model.macAndVlanEntries()->push_back(macAndVlanDetails);
    }
    return model;
  }
};

} // namespace facebook::fboss
