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
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/route/utils.h"
#include "fboss/cli/fboss2/commands/show/teflow/gen-cpp2/model_types.h"

namespace facebook::fboss {

struct CmdShowTeFlowTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IPV6_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = cli::ShowTeFlowEntryModel;
};

class CmdShowTeFlow : public CmdHandler<CmdShowTeFlow, CmdShowTeFlowTraits> {
 public:
  using NextHopThrift = facebook::fboss::NextHopThrift;
  using TeFlowDetails = facebook::fboss::TeFlowDetails;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedPrefixEntries);
  void printOutput(const RetType& model, std::ostream& out = std::cout);
  RetType createModel(
      std::vector<facebook::fboss::TeFlowDetails>& flowEntries,
      std::map<int32_t, facebook::fboss::PortInfoThrift> portInfo,
      const ObjectArgType& queriedPrefixEntries);
};

} // namespace facebook::fboss
