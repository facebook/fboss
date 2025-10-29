/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "CmdShowArp.h"

#include <fboss/agent/if/gen-cpp2/ctrl_constants.h>
#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <fmt/format.h>
#include <folly/Conv.h>
#include <folly/IPAddress.h>
#include <folly/Range.h>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

using ObjectArgType = CmdShowArpTraits::ObjectArgType;
using RetType = CmdShowArpTraits::RetType;

RetType CmdShowArp::queryClient(const HostInfo& hostInfo) {
  std::vector<facebook::fboss::ArpEntryThrift> entries;
  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
  std::map<int64_t, cfg::DsfNode> dsfNodes;
  auto client =
      utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

  client->sync_getArpTable(entries);
  client->sync_getAllPortInfo(portEntries);
  try {
    // TODO: Remove try catch once wedge_agent with getDsfNodes API
    // is rolled out
    client->sync_getDsfNodes(dsfNodes);
  } catch (const std::exception&) {
  }
  return createModel(entries, portEntries, dsfNodes);
}

std::unordered_map<std::string, std::vector<std::string>>
CmdShowArp::getAcceptedFilterValues() {
  return {{"state", {"REACHABLE", "UNREACHABLE"}}};
}

void CmdShowArp::printOutput(const RetType& model, std::ostream& out) {
  constexpr auto fmtString =
      "{:<22}{:<19}{:<12}{:<19}{:<14}{:<9}{:<12}{:<45}\n";

  out << fmt::format(
      fmtString,
      "IP Address",
      "MAC Address",
      "Interface",
      "VLAN",
      "State",
      "TTL",
      "CLASSID",
      "Voq Switch");

  for (const auto& entry : model.arpEntries().value()) {
    out << fmt::format(
        fmtString,
        entry.ip().value(),
        entry.mac().value(),
        entry.ifName().value(),
        entry.vlan().value(),
        entry.state().value(),
        folly::copy(entry.ttl().value()),
        folly::copy(entry.classID().value()),
        entry.switchName().value());
  }
  out << std::endl;
}

RetType CmdShowArp::createModel(
    std::vector<facebook::fboss::ArpEntryThrift>& arpEntries,
    std::map<int32_t, facebook::fboss::PortInfoThrift>& portEntries,
    const std::map<int64_t, cfg::DsfNode>& dsfNodes) {
  RetType model;

  for (const auto& entry : arpEntries) {
    cli::ArpEntry arpDetails;
    auto vlan = entry.vlanName().value();
    if (folly::copy(entry.vlanID().value()) != ctrl_constants::NO_VLAN()) {
      vlan += folly::to<std::string>(
          " (", folly::copy(entry.vlanID().value()), ")");
    }

    auto ip = folly::IPAddress::fromBinary(
        folly::ByteRange(
            folly::StringPiece(entry.ip().value().addr().value())));
    arpDetails.ip() = ip.str();
    arpDetails.mac() = entry.mac().value();
    arpDetails.port() = folly::copy(entry.port().value());
    arpDetails.vlan() = vlan;
    arpDetails.state() = entry.state().value();
    arpDetails.ttl() = folly::copy(entry.ttl().value());
    arpDetails.classID() = folly::copy(entry.classID().value());
    if (*entry.isLocal()) {
      arpDetails.ifName() =
          portEntries[folly::copy(entry.port().value())].name().value();
    } else {
      arpDetails.ifName() =
          folly::to<std::string>(folly::copy(entry.port().value()));
    }
    arpDetails.switchName() = "--";
    if (entry.switchId().has_value()) {
      auto ditr = dsfNodes.find(*entry.switchId());
      arpDetails.switchName() = ditr != dsfNodes.end()
          ? folly::to<std::string>(
                *ditr->second.name(), " (", *entry.switchId(), ")")
          : folly::to<std::string>(*entry.switchId());
    }

    model.arpEntries()->push_back(arpDetails);
  }
  return model;
}

} // namespace facebook::fboss
