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

#include <re2/re2.h>
#include <string>
#include <unordered_set>
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/dsfnodes/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "folly/container/Access.h"

namespace facebook::fboss {

using utils::Table;

struct CmdShowDsfNodesTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowDsfNodesModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowDsfNodes
    : public CmdHandler<CmdShowDsfNodes, CmdShowDsfNodesTraits> {
 public:
  using RetType = CmdShowDsfNodesTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo) {
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    std::map<int64_t, cfg::DsfNode> entries;
    client->sync_getDsfNodes(entries);
    return createModel(entries);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;
    table.setHeader({
        "Name",
        "Switch Id",
        "Type",
        "System port range",
    });

    for (auto const& entry : model.get_dsfNodes()) {
      table.addRow({
          *entry.name(),
          folly::to<std::string>(*entry.switchId()),
          *entry.type(),
          *entry.systemPortRange(),
      });
    }
    out << table << std::endl;
  }

  RetType createModel(std::map<int64_t, cfg::DsfNode> dsfNodes) {
    RetType model;
    const std::string kUnavail;
    for (const auto& idAndNode : dsfNodes) {
      const auto& node = idAndNode.second;
      cli::DsfNodeEntry entry;
      entry.name() = *node.name();
      entry.switchId() = *node.switchId();
      entry.type() =
          (node.type() == cfg::DsfNodeType::INTERFACE_NODE ? "Intf Node"
                                                           : "Fabric Node");
      if (node.systemPortRange()) {
        entry.systemPortRange() = folly::sformat(
            "({}, {})",
            *node.systemPortRange()->minimum(),
            *node.systemPortRange()->maximum());
      } else {
        entry.systemPortRange() = "--";
      }
      model.dsfNodes()->push_back(entry);
    }
    return model;
  }
};

} // namespace facebook::fboss
