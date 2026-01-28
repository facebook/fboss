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

#include "fboss/agent/AddressUtil.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/mirror/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

#include "folly/json/json.h"

namespace facebook::fboss {

using utils::Table;

struct CmdShowMirrorTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_MIRROR_LIST;
  using ObjectArgType = utils::MirrorList;
  using RetType = cli::ShowMirrorModel;
};

class CmdShowMirror : public CmdHandler<CmdShowMirror, CmdShowMirrorTraits> {
 public:
  using ObjectArgType = CmdShowMirrorTraits::ObjectArgType;
  using RetType = CmdShowMirrorTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedMirrors);

  void printOutput(const RetType& model, std::ostream& out = std::cout);

  std::string getEgressPortName(
      const std::string& egressPort,
      const std::map<int32_t, PortInfoThrift>& portInfoEntries);

  std::string getIPAddressStr(const std::string& rawIPAddress);

  void processTunnel(
      const folly::dynamic& tunnel,
      cli::ShowMirrorModelEntry& mirrorDetails);

  void processWithNoTunnel(
      const folly::dynamic& mirrorMapEntry,
      cli::ShowMirrorModelEntry& mirrorDetails);

  RetType createModel(
      const std::string& mirrorMaps,
      const std::map<int32_t, PortInfoThrift>& portInfoEntries,
      const ObjectArgType& queriedMirrors);
};

} // namespace facebook::fboss
