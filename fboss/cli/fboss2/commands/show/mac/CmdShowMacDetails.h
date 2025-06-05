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
#include <cstdint>
#include <map>
#include <vector>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/mac/gen-cpp2/model_types.h"

namespace facebook::fboss {

struct CmdShowMacDetailsTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = utils::NoneArgType;
  using RetType = cli::ShowMacDetailsModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowMacDetails
    : public CmdHandler<CmdShowMacDetails, CmdShowMacDetailsTraits> {
 public:
  using RetType = CmdShowMacDetailsTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);
  void printOutput(const RetType& model, std::ostream& out = std::cout);
  RetType createModel(
      std::vector<facebook::fboss::L2EntryThrift>& l2Entries,
      std::map<int32_t, facebook::fboss::PortInfoThrift>& portEntries,
      std::vector<facebook::fboss::AggregatePortThrift>& aggregatePortEntries);
};

} // namespace facebook::fboss
