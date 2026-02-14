/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/flowlet/CmdShowFlowlet.h"

namespace facebook::fboss {

CmdShowFlowlet::RetType CmdShowFlowlet::queryClient(const HostInfo& hostInfo) {
  std::vector<EcmpDetails> entries;
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);

  client->sync_getAllEcmpDetails(entries);
  return createModel(entries);
}

void CmdShowFlowlet::printOutput(const RetType& model, std::ostream& out) {
  for (const auto& entry : model.ecmpEntries().value()) {
    out << fmt::format("  ECMP ID: {}\n", folly::copy(entry.ecmpId().value()));
    out << fmt::format(
        "    Flowlet Enabled: {}\n",
        folly::copy(entry.flowletEnabled().value()));
    out << fmt::format(
        "    Flowlet Interval: {}\n",
        folly::copy(entry.flowletInterval().value()));
    out << fmt::format(
        "    Flowlet Table Size: {}\n",
        folly::copy(entry.flowletTableSize().value()));
  }
}

CmdShowFlowlet::RetType CmdShowFlowlet::createModel(
    std::vector<facebook::fboss::EcmpDetails>& ecmpEntries) {
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

} // namespace facebook::fboss
