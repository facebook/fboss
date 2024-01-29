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
#include <folly/String.h>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/flowlet/gen-cpp2/model_types.h"

namespace facebook::fboss {

struct CmdShowFlowletTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = utils::NoneArgType;
  using RetType = cli::ShowEcmpEntryModel;
};

class CmdShowFlowlet : public CmdHandler<CmdShowFlowlet, CmdShowFlowletTraits> {
 public:
  using EcmpDetails = facebook::fboss::EcmpDetails;

  RetType queryClient(const HostInfo& hostInfo) {
    std::vector<EcmpDetails> entries;
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);

    client->sync_getAllEcmpDetails(entries);
    return createModel(entries);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    for (const auto& entry : model.get_ecmpEntries()) {
      out << fmt::format("  ECMP ID: {}\n", entry.get_ecmpId());
      out << fmt::format(
          "    Flowlet Enabled: {}\n", entry.get_flowletEnabled());
      out << fmt::format(
          "    Flowlet Interval: {}\n", entry.get_flowletInterval());
      out << fmt::format(
          "    Flowlet Table Size: {}\n", entry.get_flowletTableSize());
    }
  }

  RetType createModel(std::vector<facebook::fboss::EcmpDetails>& ecmpEntries) {
    RetType model;

    for (const auto& entry : ecmpEntries) {
      cli::EcmpEntry ecmpEntry;
      ecmpEntry.ecmpId() = *(entry.ecmpId());
      ecmpEntry.flowletEnabled() = *(entry.flowletEnabled());
      ecmpEntry.flowletInterval() = *(entry.flowletInterval());
      ecmpEntry.flowletTableSize() = *(entry.flowletTableSize());
      model.ecmpEntries()->emplace_back(ecmpEntry);
    }
    return model;
  }
};

} // namespace facebook::fboss
