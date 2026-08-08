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
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/route/CmdShowRoute.h"
#include "fboss/cli/fboss2/commands/show/route/gen-cpp2/model_types.h"

namespace facebook::fboss {

struct CmdShowRouteStaticTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ParentCmd = CmdShowRoute;
  using ObjectArgType = utils::NoneArgType;
  using RetType = cli::ShowRouteModel;
};

class CmdShowRouteStatic
    : public CmdHandler<CmdShowRouteStatic, CmdShowRouteStaticTraits> {
 public:
  // Which address family to include when building the model.
  enum class AddressFamily { ALL, V4, V6 };

  RetType queryClient(const HostInfo& hostInfo);
  void printOutput(const RetType& model, std::ostream& out = std::cout);

  // Shared by the ip/ipv6 subcommands. Filters the static route table to the
  // requested address family and converts it into the CLI model.
  static RetType createModel(
      const std::vector<facebook::fboss::RouteDetails>& routeEntries,
      AddressFamily family);

  // Shared by the ip/ipv6 subcommands.
  static void printRouteModel(const RetType& model, std::ostream& out);

  // Queries the agent for routes that have an entry installed by the
  // STATIC_ROUTE client. Uses getRouteTableDetails (rather than
  // getRouteTableByClient) because RouteDetails carries the forwarding action
  // needed to distinguish null0 (Drop) from cpu (ToCPU) routes.
  static std::vector<facebook::fboss::RouteDetails> queryStaticRoutes(
      const HostInfo& hostInfo);
};

} // namespace facebook::fboss
