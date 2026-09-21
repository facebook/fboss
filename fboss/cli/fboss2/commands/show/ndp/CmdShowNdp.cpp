/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "CmdShowNdp.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fboss/agent/if/gen-cpp2/ctrl_constants.h>
#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <folly/Conv.h>
#include <folly/IPAddress.h>
#include <folly/Range.h>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

using utils::Table;
using ObjectArgType = CmdShowNdpTraits::ObjectArgType;
using RetType = CmdShowNdpTraits::RetType;

RetType CmdShowNdp::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& queriedNdpEntries) {
  std::vector<facebook::fboss::NdpEntryThrift> entries;
  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
  std::map<int64_t, cfg::DsfNode> dsfNodes;
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);

  client->sync_getNdpTable(entries);
  client->sync_getAllPortInfo(portEntries);
  try {
    client->sync_getDsfNodes(dsfNodes);
  } catch (const std::exception&) {
    // getDsfNodes is not supported on non-DSF switches (e.g. wedge400)
  }
  return createModel(entries, queriedNdpEntries, portEntries, dsfNodes);
}

void CmdShowNdp::printOutput(const RetType& model, std::ostream& out) {
  Table table;
  table.setHeader(
      {"IP Address",
       "MAC Address",
       "Interface",
       "VLAN/InterfaceID",
       "State",
       "TTL",
       "CLASSID",
       "Voq Switch",
       "Resolved Since"});

  for (const auto& entry : model.ndpEntries().value()) {
    auto vlan = entry.vlanName().value();
    if (folly::copy(entry.vlanID().value()) != ctrl_constants::NO_VLAN()) {
      vlan += folly::to<std::string>(
          " (", folly::copy(entry.vlanID().value()), ")");
    }
    table.addRow(
        {entry.ip().value(),
         entry.mac().value(),
         entry.port().value(),
         vlan,
         entry.state().value(),
         std::to_string(folly::copy(entry.ttl().value())),
         std::to_string(folly::copy(entry.classID().value())),
         entry.switchName().value(),
         entry.resolvedSince().value()});
  }
  out << table << std::endl;
}

RetType CmdShowNdp::createModel(
    const std::vector<facebook::fboss::NdpEntryThrift>& ndpEntries,
    const ObjectArgType& queriedNdpEntries,
    std::map<int32_t, facebook::fboss::PortInfoThrift>& portEntries,
    const std::map<int64_t, cfg::DsfNode>& dsfNodes) {
  RetType model;
  std::unordered_set<std::string> queriedSet(
      queriedNdpEntries.begin(), queriedNdpEntries.end());

  for (const auto& entry : ndpEntries) {
    auto ip = folly::IPAddress::fromBinary(
        folly::ByteRange(
            folly::StringPiece(entry.ip().value().addr().value())));

    if (queriedNdpEntries.size() == 0 || queriedSet.count(ip.str())) {
      cli::NdpEntry ndpDetails;
      ndpDetails.ip() = ip.str();
      ndpDetails.mac() = entry.mac().value();
      if (*entry.isLocal()) {
        ndpDetails.port() =
            portEntries[folly::copy(entry.port().value())].name().value();
      } else {
        ndpDetails.port() =
            folly::to<std::string>(folly::copy(entry.port().value()));
      }
      ndpDetails.vlanName() = entry.vlanName().value();
      // interfaceID is always populated since intf_nbr_tables is enabled.
      ndpDetails.vlanID() = folly::copy(entry.interfaceID().value());
      ndpDetails.state() = entry.state().value();
      ndpDetails.ttl() = folly::copy(entry.ttl().value());
      ndpDetails.classID() = folly::copy(entry.classID().value());
      ndpDetails.switchName() = "--";
      if (entry.switchId().has_value()) {
        auto ditr = dsfNodes.find(*entry.switchId());
        ndpDetails.switchName() = ditr != dsfNodes.end()
            ? folly::to<std::string>(
                  *ditr->second.name(), " (", *entry.switchId(), ")")
            : folly::to<std::string>(*entry.switchId());
      }
      ndpDetails.resolvedSince() = "--";
      if (entry.resolvedSince().has_value()) {
        time_t timestamp = static_cast<time_t>(entry.resolvedSince().value());
        std::tm tm{};
        localtime_r(&timestamp, &tm);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %T");
        ndpDetails.resolvedSince() = oss.str();
      }

      model.ndpEntries()->push_back(ndpDetails);
    }
  }
  return model;
}

std::string_view CmdShowNdpTraits::description() {
  return "Displays the switch's IPv6 NDP neighbor table: each resolved IPv6 neighbor, the interface it was learned on, its VLAN/interface ID, reachability state, and TTL. Use it to confirm IPv6 next-hops are present before debugging routing.";
}

CmdShowNdp::RetType CmdShowNdp::sampleModel() {
  RetType model;

  cli::NdpEntry entry1;
  entry1.ip() = "fe80::200:11ff:fe22:3301";
  entry1.mac() = "02:00:11:22:33:01";
  entry1.port() = "eth1/1/1";
  entry1.vlanName() = "vlan2001";
  entry1.vlanID() = 2001;
  entry1.state() = "REACHABLE";
  entry1.ttl() = 3814;
  entry1.classID() = 0;
  entry1.switchName() = "--";
  entry1.resolvedSince() = "--";

  cli::NdpEntry entry2;
  entry2.ip() = "2001:db8:a::3e";
  entry2.mac() = "02:00:11:22:33:01";
  entry2.port() = "eth1/1/1";
  entry2.vlanName() = "vlan2001";
  entry2.vlanID() = 2001;
  entry2.state() = "REACHABLE";
  entry2.ttl() = 36122;
  entry2.classID() = 0;
  entry2.switchName() = "--";
  entry2.resolvedSince() = "--";

  cli::NdpEntry entry3;
  entry3.ip() = "fe80::200:11ff:fe22:3302";
  entry3.mac() = "02:00:11:22:33:02";
  entry3.port() = "eth1/2/1";
  entry3.vlanName() = "vlan2003";
  entry3.vlanID() = 2003;
  entry3.state() = "REACHABLE";
  entry3.ttl() = 32225;
  entry3.classID() = 0;
  entry3.switchName() = "--";
  entry3.resolvedSince() = "--";

  model.ndpEntries() = {entry1, entry2, entry3};
  return model;
}

// Explicit template instantiation
template void CmdHandler<CmdShowNdp, CmdShowNdpTraits>::run();
template const ValidFilterMapType
CmdHandler<CmdShowNdp, CmdShowNdpTraits>::getValidFilters();

} // namespace facebook::fboss
