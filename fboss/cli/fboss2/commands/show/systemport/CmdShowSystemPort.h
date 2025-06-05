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

#include "fboss/cli/fboss2/CmdHandler.h"

#include <thrift/lib/cpp/transport/TTransportException.h>
#include <map>
#include <string>
#include "fboss/cli/fboss2/commands/show/systemport/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/commands/show/systemport/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdShowSystemPortTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_SYSTEM_PORT_LIST;
  using ObjectArgType = utils::SystemPortList;
  using RetType = cli::ShowSystemPortModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowSystemPort
    : public CmdHandler<CmdShowSystemPort, CmdShowSystemPortTraits> {
 public:
  using ObjectArgType = CmdShowSystemPortTraits::ObjectArgType;
  using RetType = CmdShowSystemPortTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedSystemPorts);

  void printOutput(const RetType& model, std::ostream& out = std::cout);

  RetType createModel(
      std::map<int64_t, facebook::fboss::SystemPortThrift> systemPortEntries,
      const ObjectArgType& queriedSystemPorts,
      std::map<std::string, facebook::fboss::HwSysPortStats>&
          systemportHwStats);
};

} // namespace facebook::fboss
