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
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/route/CmdShowRoute.h"
#include "fboss/cli/fboss2/commands/show/route/gen-cpp2/model_types.h"

namespace facebook::fboss {

struct CmdShowRouteSummaryTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ParentCmd = CmdShowRoute;
  using ObjectArgType = utils::NoneArgType;
  using RetType = cli::ShowRouteSummaryModel;
};

class CmdShowRouteSummary
    : public CmdHandler<CmdShowRouteSummary, CmdShowRouteSummaryTraits> {
 public:
  RetType queryClient(const HostInfo& hostInfo);
  void printOutput(const RetType& model, std::ostream& out = std::cout);
  RetType createModel(
      const std::vector<facebook::fboss::UnicastRoute>& routeEntries);
};

} // namespace facebook::fboss
