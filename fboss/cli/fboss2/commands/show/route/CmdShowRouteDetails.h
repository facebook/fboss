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
#include "fboss/cli/fboss2/commands/show/route/CmdShowRoute.h"
#include "fboss/cli/fboss2/commands/show/route/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/commands/show/route/utils.h"

namespace facebook::fboss {

struct CmdShowRouteDetailsTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IP_LIST;
  using ParentCmd = CmdShowRoute;
  using ObjectArgType = utils::IPList;
  using RetType = cli::ShowRouteDetailsModel;
};

class CmdShowRouteDetails
    : public CmdHandler<CmdShowRouteDetails, CmdShowRouteDetailsTraits> {
 private:
  std::map<std::string, std::string> vlanAggregatePortMap;
  std::map<std::string, std::map<std::string, std::vector<std::string>>>
      vlanPortMap;

  std::string parseRootPort(const std::string& str);
  void populateAggregatePortMap(
      const std::unique_ptr<apache::thrift::Client<FbossCtrl>>& client);
  void populateVlanPortMap(
      const std::unique_ptr<apache::thrift::Client<FbossCtrl>>& client);

 public:
  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedRoutes);
  void printOutput(const RetType& model, std::ostream& out = std::cout);
  RetType createModel(
      std::vector<facebook::fboss::RouteDetails>& routeEntries,
      const ObjectArgType& queriedRoutes);
  std::string getClassID(cfg::AclLookupClass classID);
};

} // namespace facebook::fboss
