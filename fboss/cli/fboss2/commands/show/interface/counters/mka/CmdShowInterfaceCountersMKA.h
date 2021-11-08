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
#include "fboss/cli/fboss2/commands/show/interface/counters/CmdShowInterfaceCounters.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/mka/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/Table.h"

#include <boost/algorithm/string.hpp>
#include <unordered_set>

namespace facebook::fboss {

using utils::Table;

struct CmdShowInterfaceCountersMKATraits : public BaseCommandTraits {
  using ParentCmd = CmdShowInterfaceCounters;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::InterfaceCountersMKAModel;
};

class CmdShowInterfaceCountersMKA : public CmdHandler<
                                        CmdShowInterfaceCountersMKA,
                                        CmdShowInterfaceCountersMKATraits> {
 public:
  using ObjectArgType = CmdShowInterfaceCountersMKATraits::ObjectArgType;
  using RetType = CmdShowInterfaceCountersMKATraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& /*queriedIfs*/) {
    auto client =
        utils::createClient<facebook::fboss::QsfpServiceAsyncClient>(hostInfo);

    return createModel();
  }

  RetType createModel() {
    RetType ret;
    return ret;
  }

  void printOutput(const RetType& /*model*/, std::ostream& out = std::cout) {
    Table table;
    out << "NOT implemented yet!" << std::endl;
  }
};

} // namespace facebook::fboss
