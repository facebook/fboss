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

#include <map>
#include <string>
#include <vector>
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/lldp/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

struct CmdShowLldpTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = cli::ShowLldpModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowLldp : public CmdHandler<CmdShowLldp, CmdShowLldpTraits> {
 public:
  using RetType = CmdShowLldpTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs);

  void printOutput(const RetType& model, std::ostream& out = std::cout);

  const facebook::fboss::PortInfoThrift getPortInfo(
      int32_t portId,
      std::map<int32_t, facebook::fboss::PortInfoThrift>& portEntries);

  bool doPeersMatch(
      const std::string& expectedPeer,
      const std::string& actualPeer);

  utils::Table::Style get_PeerStyle(
      const std::string& expectedPeer,
      const std::string& actualPeer);

  utils::Table::Style get_ExpectedPeerStyle(const std::string& expectedPeer);

  utils::Table::Style get_StatusStyle(std::string status_text);

  std::string extractExpectedPort(const std::string& portDescription);

  RetType createModel(
      std::vector<facebook::fboss::LinkNeighborThrift> lldpEntries,
      std::map<int32_t, facebook::fboss::PortInfoThrift>& portEntries,
      const std::vector<std::string>& queriedIfs);
};

} // namespace facebook::fboss
