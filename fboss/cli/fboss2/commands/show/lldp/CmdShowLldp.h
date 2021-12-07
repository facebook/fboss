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

#include <boost/algorithm/string.hpp>
#include <re2/re2.h>
#include <string>
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/lldp/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

using utils::Table;

struct CmdShowLldpTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowLldpModel;
};

class CmdShowLldp : public CmdHandler<CmdShowLldp, CmdShowLldpTraits> {
 public:
  using RetType = CmdShowLldpTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo) {
    std::vector<facebook::fboss::LinkNeighborThrift> entries;
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    client->sync_getLldpNeighbors(entries);

    std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;

    client->sync_getAllPortInfo(portEntries);

    return createModel(entries, portEntries);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
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

    for (auto const& entry : model.get_lldpEntries()) {
      table.addRow({
          entry.get_localPort(),
          Table::StyledCell(
              entry.get_status(), get_StatusStyle(entry.get_status())),
          Table::StyledCell(entry.get_expectedPeer(), Table::Style::INFO),
          Table::StyledCell(
              removeFbDomains(entry.get_systemName()),
              get_PeerStyle(entry.get_expectedPeer(), entry.get_systemName())),
          entry.get_remotePort(),
          entry.get_remotePlatform(),
          entry.get_remotePortDescription(),
      });
    }

    out << table << std::endl;
  }

  const facebook::fboss::PortInfoThrift getPortInfo(
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
      if (portId == port.get_portId()) {
        returnPort = port;
      }
    }
    return returnPort;
  }

  const std::string removeFbDomains(const std::string& hostname) {
    // Simple helper function to remove FQDN
    std::string host_copy = hostname;
    const RE2 fb_domains(".facebook.com$|.tfbnw.net$");
    RE2::Replace(&host_copy, fb_domains, "");
    return host_copy;
  }

  bool doPeersMatch(
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

  Table::Style get_PeerStyle(
      const std::string& expectedPeer,
      const std::string& actualPeer) {
    if (doPeersMatch(expectedPeer, actualPeer)) {
      return Table::Style::GOOD;
    } else {
      return Table::Style::ERROR;
    }
  }

  Table::Style get_StatusStyle(std::string status_text) {
    if (status_text == "up") {
      return Table::Style::GOOD;
    } else {
      return Table::Style::ERROR;
    }
  }

  std::string extractExpectedPort(const std::string& portDescription) {
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

    This function uses the BOOST split function to parse out the hostname
    portion to be used as the expected peer
    */
    std::vector<std::string> results;
    // First try splitting as if it's a minipack
    boost::split(results, portDescription, [](char c) { return c == ' '; });
    // If we don't get results from the split then split on "." which is a
    // different format
    if (results.size() <= 1) {
      boost::split(results, portDescription, [](char c) { return c == '.'; });
      // Finally, if it's an RSW we need to handle that differently
      const RE2 fsw_regex("^fsw.*");
      if (RE2::FullMatch(portDescription, fsw_regex)) {
        return results[0] + "." + results[1];
      }
    }
    return results[1];
  }

  RetType createModel(
      std::vector<facebook::fboss::LinkNeighborThrift> lldpEntries,
      std::map<int32_t, facebook::fboss::PortInfoThrift>& portEntries) {
    RetType model;

    for (const auto& entry : lldpEntries) {
      cli::LldpEntry lldpDetails;

      const auto portInfo = getPortInfo(entry.get_localPort(), portEntries);
      const auto operState = portInfo.get_operState();

      const auto expected_peer =
          extractExpectedPort(portInfo.get_description());

      lldpDetails.localPort_ref() = *entry.get_localPortName();
      lldpDetails.systemName_ref() = entry.get_systemName()
          ? *entry.get_systemName()
          : entry.get_printableChassisId();
      lldpDetails.remotePort_ref() = entry.get_printablePortId();
      lldpDetails.remotePlatform_ref() = *entry.get_systemDescription();
      lldpDetails.remotePortDescription_ref() = portInfo.get_description();
      lldpDetails.status_ref() =
          (operState == facebook::fboss::PortOperState::UP) ? "up" : "down";
      lldpDetails.expectedPeer_ref() = expected_peer;

      model.lldpEntries_ref()->push_back(lldpDetails);
    }

    std::sort(
        model.lldpEntries_ref()->begin(),
        model.lldpEntries_ref()->end(),
        [](cli::LldpEntry& a, cli::LldpEntry b) {
          return a.get_localPort() < b.get_localPort();
        });

    return model;
  }
};

} // namespace facebook::fboss
