/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "CmdShowLldp.h"

#include <re2/re2.h>
#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

using utils::Table;
using RetType = CmdShowLldpTraits::RetType;

RetType CmdShowLldp::queryClient(
    const HostInfo& hostInfo,
    const std::vector<std::string>& queriedIfs) {
  std::vector<facebook::fboss::LinkNeighborThrift> entries;
  auto client =
      utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

  client->sync_getLldpNeighbors(entries);

  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;

  client->sync_getAllPortInfo(portEntries);

  return createModel(entries, portEntries, queriedIfs);
}

void CmdShowLldp::printOutput(const RetType& model, std::ostream& out) {
  Table table;
  table.setHeader({
      "Local Int",
      "Status",
      "Expected-Peer",
      "LLDP-Peer",
      "Peer-Int",
      "Platform",
      "Peer Description",
  });

  for (auto const& entry : model.lldpEntries().value()) {
    std::string expectedPeerDisplay = entry.expectedPeer().value();
    if (expectedPeerDisplay.empty()) {
      expectedPeerDisplay = "EMPTY";
    }

    table.addRow({
        entry.localPort().value(),
        Table::StyledCell(
            entry.status().value(), get_StatusStyle(entry.status().value())),
        Table::StyledCell(
            expectedPeerDisplay,
            get_ExpectedPeerStyle(entry.expectedPeer().value())),
        Table::StyledCell(
            utils::removeFbDomains(entry.systemName().value()),
            get_PeerStyle(
                entry.expectedPeer().value(), entry.systemName().value())),
        entry.remotePort().value(),
        entry.remotePlatform().value(),
        entry.remotePortDescription().value(),
    });
  }

  out << table << std::endl;
}

const facebook::fboss::PortInfoThrift CmdShowLldp::getPortInfo(
    int32_t portId,
    std::map<int32_t, facebook::fboss::PortInfoThrift>& portEntries) {
  /*
  The LinkNeighborThrift object does not give us everything we would want to
  know about the local port, specifically the description of the local port.
  The description of the local port should contain information about the
  expected remote port.

  This method loops through all of the local ports and compares portID to find
  the PortInfoThrift object that corresponds to the one being processed by
  createModel.

  It returns that PortInfoThrift object for inclusion in the this command
  model
  */
  facebook::fboss::PortInfoThrift returnPort;
  for (const auto& entry : portEntries) {
    const auto port = entry.second;
    if (portId == folly::copy(port.portId().value())) {
      returnPort = port;
    }
  }
  return returnPort;
}

bool CmdShowLldp::doPeersMatch(
    const std::string& expectedPeer,
    const std::string& actualPeer) {
  /*
  Helper function to see if the expected peer that was derived from the
  port description actually matches the shortname of the peer that was
  discovered via LLDP
  */
  const RE2 peer_regex("^" + expectedPeer + ".*");
  return RE2::FullMatch(actualPeer, peer_regex);
}

Table::Style CmdShowLldp::get_PeerStyle(
    const std::string& expectedPeer,
    const std::string& actualPeer) {
  if (doPeersMatch(expectedPeer, actualPeer)) {
    return Table::Style::GOOD;
  } else {
    return Table::Style::ERROR;
  }
}

Table::Style CmdShowLldp::get_ExpectedPeerStyle(
    const std::string& expectedPeer) {
  if (expectedPeer.empty()) {
    return Table::Style::WARN;
  } else {
    return Table::Style::INFO;
  }
}

Table::Style CmdShowLldp::get_StatusStyle(std::string status_text) {
  if (status_text == "up") {
    return Table::Style::GOOD;
  } else {
    return Table::Style::ERROR;
  }
}

std::string CmdShowLldp::extractExpectedPort(
    const std::string& portDescription) {
  /*
  Network interfaces have descriptions that indicated the expected remote
  peer. Unfortunately we seem to have at least 3 different standards for
  interface descriptions depending upon platform.  Examples noted below:

  Minipack/Minipack2:
   u-013: ssw013.s001 (F=spine:L=d-050)
   d-001: rsw001.p050 (F=rack:L=u-001:D=vll2.0001.0050.0001)

  Glacier/Glacier2:
   et6-1-1.fa001-du001.ftw5:T=usIF:U=facebook
   d-001: fsw001.p001 (F=fabric:L=u-001)

  Wedge:
   fsw001.p062.f01.vll3:eth4/1/1

  This function uses the folly::split function to parse out the hostname
  portion to be used as the expected peer
  */
  std::vector<std::string> results;
  // First try splitting as if it's a minipack
  folly::split(' ', portDescription, results);
  if (results.size() > 1) {
    return results[1];
  }

  // If we don't get results from the split then split on "." which is a
  // different format
  folly::split('.', portDescription, results);
  // Depending on which tier the peer is, we need to grab the specific
  // fields to make a useable hostname
  const RE2 ssw_regex(".*ssw.*");
  if (RE2::FullMatch(portDescription, ssw_regex)) {
    return results[1] + "." + results[2];
  }
  const RE2 fsw_regex("^fsw.*");
  if (RE2::FullMatch(portDescription, fsw_regex)) {
    results.clear();
    folly::split(':', portDescription, results);
    // Prints as fsw001.p062.f01.vll3
    return results[0];
  }
  const RE2 ctsw_regex("^ctsw.*");
  if (RE2::FullMatch(portDescription, ctsw_regex)) {
    return results[0] + "." + results[1];
  }
  const RE2 rsw_regex("^rsw.*");
  if (RE2::FullMatch(portDescription, rsw_regex)) {
    return results[0] + "." + results[1];
  }
  const RE2 rusw_regex("^rusw.*");
  if (RE2::FullMatch(portDescription, rusw_regex)) {
    return results[0] + "." + results[1];
  }
  const RE2 rtsw_regex("^rtsw.*");
  if (RE2::FullMatch(portDescription, rtsw_regex)) {
    return results[0] + "." + results[1];
  }
  const RE2 resw_regex("^resw.*");
  if (RE2::FullMatch(portDescription, resw_regex)) {
    return results[0] + "." + results[1];
  }
  const RE2 xsw_regex(".*xsw.*");
  if (RE2::FullMatch(portDescription, xsw_regex)) {
    return results[2] + "." + results[3];
  }
  const RE2 bag_regex(".*\\.bag([^:]+)");
  if (RE2::PartialMatch(portDescription, bag_regex)) {
    std::string match;
    RE2::PartialMatch(portDescription, bag_regex, &match);
    return "bag" + match;
  }
  const RE2 bc_regex(".*\\.bc([^:]+)");
  if (RE2::PartialMatch(portDescription, bc_regex)) {
    std::string match;
    RE2::PartialMatch(portDescription, bc_regex, &match);
    return "bc" + match;
  }

  // default to empty string, entire port descriptions is in a separate column
  return "";
}

RetType CmdShowLldp::createModel(
    std::vector<facebook::fboss::LinkNeighborThrift> lldpEntries,
    std::map<int32_t, facebook::fboss::PortInfoThrift>& portEntries,
    const std::vector<std::string>& queriedIfs) {
  RetType model;

  std::unordered_set<std::string> queriedSet(
      queriedIfs.begin(), queriedIfs.end());

  for (const auto& entry : lldpEntries) {
    cli::LldpEntry lldpDetails;

    const auto portInfo =
        getPortInfo(folly::copy(entry.localPort().value()), portEntries);
    if (queriedIfs.size() == 0 || queriedSet.count(portInfo.name().value())) {
      const auto operState = folly::copy(portInfo.operState().value());
      const auto expected_peer = portInfo.expectedLLDPeerName().has_value()
          ? portInfo.expectedLLDPeerName().value()
          : extractExpectedPort(portInfo.description().value());
      if (auto localPortName =
              apache::thrift::get_pointer(entry.localPortName())) {
        lldpDetails.localPort() = *localPortName;
      }
      lldpDetails.systemName() = apache::thrift::get_pointer(entry.systemName())
          ? *apache::thrift::get_pointer(entry.systemName())
          : entry.printableChassisId().value();
      lldpDetails.remotePort() = entry.printablePortId().value();
      if (apache::thrift::get_pointer(entry.systemDescription())) {
        lldpDetails.remotePlatform() =
            *apache::thrift::get_pointer(entry.systemDescription());
      }
      lldpDetails.remotePortDescription() = portInfo.description().value();
      lldpDetails.status() =
          (operState == facebook::fboss::PortOperState::UP) ? "up" : "down";
      lldpDetails.expectedPeer() = expected_peer;

      model.lldpEntries()->push_back(lldpDetails);
    }
  }

  std::sort(
      model.lldpEntries()->begin(),
      model.lldpEntries()->end(),
      [](cli::LldpEntry& a, cli::LldpEntry b) {
        return utils::comparePortName(
            a.localPort().value(), b.localPort().value());
      });

  return model;
}

} // namespace facebook::fboss
