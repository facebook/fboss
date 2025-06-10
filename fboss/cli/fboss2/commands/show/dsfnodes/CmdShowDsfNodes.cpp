/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "CmdShowDsfNodes.h"

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <re2/re2.h>
#include <map>
#include <string>
#include <vector>
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "folly/Conv.h"
#include "folly/Format.h"

namespace facebook::fboss {

using utils::Table;
using RetType = CmdShowDsfNodesTraits::RetType;

RetType CmdShowDsfNodes::queryClient(const HostInfo& hostInfo) {
  auto client =
      utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);
  std::map<int64_t, cfg::DsfNode> entries;
  client->sync_getDsfNodes(entries);
  return createModel(entries);
}

void CmdShowDsfNodes::printOutput(const RetType& model, std::ostream& out) {
  Table table;
  table.setHeader({
      "Name",
      "Switch Id",
      "Type",
      "System port ranges",
  });

  for (auto const& entry : model.dsfNodes().value()) {
    table.addRow({
        *entry.name(),
        folly::to<std::string>(*entry.switchId()),
        *entry.type(),
        *entry.systemPortRanges(),
    });
  }
  out << table << std::endl;
}

RetType CmdShowDsfNodes::createModel(
    const std::map<int64_t, cfg::DsfNode>& dsfNodes) {
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
    std::vector<std::string> ranges;
    if (node.systemPortRanges()->systemPortRanges()->size()) {
      for (const auto& range : *node.systemPortRanges()->systemPortRanges()) {
        ranges.push_back(
            folly::sformat("({}, {})", *range.minimum(), *range.maximum()));
      }
      entry.systemPortRanges() = folly::join(", ", ranges);
    } else {
      entry.systemPortRanges() = "--";
    }
    model.dsfNodes()->push_back(entry);
  }
  return model;
}

} // namespace facebook::fboss
