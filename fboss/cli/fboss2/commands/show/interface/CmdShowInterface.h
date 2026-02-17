/*
 *  Copyright (c) Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

using utils::Table;

struct CmdShowInterfaceTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = cli::ShowInterfaceModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowInterface
    : public CmdHandler<CmdShowInterface, CmdShowInterfaceTraits> {
 private:
  bool isVoq = false;

 public:
  using ObjectArgType = CmdShowInterfaceTraits::ObjectArgType;
  using RetType = CmdShowInterfaceTraits::RetType;

  // PortPosition stores physical port location based on ethernet
  // port name, e.g. eth1/2/1 means ethernet port in linecard 1,
  // module 2, port 1. This will be used to sort the output.
  struct PortPosition {
    int32_t linecard;
    int32_t mod;
    int32_t port;
  };

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedIfs);
  RetType createModel(
      const HostInfo& hostInfo,
      const std::map<int32_t, facebook::fboss::PortInfoThrift>& portEntries,
      std::map<int32_t, facebook::fboss::InterfaceDetail> intfDetails,
      const std::map<int64_t, cfg::DsfNode>& dsfNodes,
      const ObjectArgType& queriedIfs);
  void populateVlanToMtu(
      std::unordered_map<int32_t, int32_t>& vlanToMtu,
      const std::map<int32_t, facebook::fboss::InterfaceDetail>& intfDetails);
  void populateVlanToPrefixes(
      std::unordered_map<int32_t, std::vector<cli::IpPrefix>>& vlanToPrefixes,
      const std::map<int32_t, facebook::fboss::InterfaceDetail>& intfDetails);
  void sortByInterfaceName(RetType& model);
  void populateNameToPortPositionMap(
      std::unordered_map<std::string, PortPosition>& nameToPortPosition,
      const std::vector<std::string>& names);
  void printOutput(const RetType& model, std::ostream& out = std::cout);
  Table::StyledCell colorStatusCell(const std::string& status);
};

} // namespace facebook::fboss
