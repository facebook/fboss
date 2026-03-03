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
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/route/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/commands/show/route/utils.h"

namespace facebook::fboss {

struct CmdShowRouteTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = utils::NoneArgType;
  using RetType = cli::ShowRouteModel;
};

class CmdShowRoute : public CmdHandler<CmdShowRoute, CmdShowRouteTraits> {
 public:
  using NextHopThrift = facebook::fboss::NextHopThrift;
  using UnicastRoute = facebook::fboss::UnicastRoute;

  RetType queryClient(const HostInfo& hostInfo);
  void printOutput(const RetType& model, std::ostream& out = std::cout);
  bool isUcmpActive(const std::vector<NextHopThrift>& nextHops);
  RetType createModel(std::vector<facebook::fboss::UnicastRoute>& routeEntries);
};

} // namespace facebook::fboss
