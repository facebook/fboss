/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "CmdShowMacAddrToBlock.h"

#include <fmt/core.h>

namespace facebook::fboss {

CmdShowMacAddrToBlock::RetType CmdShowMacAddrToBlock::queryClient(
    const HostInfo& hostInfo) {
  std::vector<cfg::MacAndVlan> entries;
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);

  client->sync_getMacAddrsToBlock(entries);
  return createModel(entries);
}

void CmdShowMacAddrToBlock::printOutput(
    const RetType& model,
    std::ostream& out) {
  constexpr auto fmtString = "{:<19}{:<19}\n";

  out << fmt::format(fmtString, "VLAN", "MAC Address");

  for (const auto& entry : model.macAndVlanEntries().value()) {
    out << fmt::format(
        fmtString,
        folly::copy(entry.vlanID().value()),
        entry.macAddress().value());
  }
  out << std::endl;
}

CmdShowMacAddrToBlock::RetType CmdShowMacAddrToBlock::createModel(
    std::vector<cfg::MacAndVlan>& macAndVlanEntries) {
  RetType model;

  for (const auto& entry : macAndVlanEntries) {
    cli::MacAndVlan macAndVlanDetails;

    macAndVlanDetails.vlanID() = folly::copy(entry.vlanID().value());
    macAndVlanDetails.macAddress() = entry.macAddress().value();

    model.macAndVlanEntries()->push_back(macAndVlanDetails);
  }
  return model;
}

} // namespace facebook::fboss
