/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "CmdShowRif.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fboss/agent/if/gen-cpp2/ctrl_constants.h>
#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <cstdint>
#include <map>
#include <string>
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "folly/IPAddress.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

namespace facebook::fboss {

using utils::Table;
using RetType = CmdShowRifTraits::RetType;

RetType CmdShowRif::queryClient(const HostInfo& hostInfo) {
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);

  std::map<int32_t, facebook::fboss::InterfaceDetail> rifs;
  client->sync_getAllInterfaces(rifs);

  return createModel(rifs);
}

RetType CmdShowRif::createModel(
    std::map<int32_t, facebook::fboss::InterfaceDetail> rifs) {
  RetType model;

  auto getRemoteIntfTypeStr = [](const auto& remoteIntfType) {
    if (remoteIntfType.has_value()) {
      switch (remoteIntfType.value()) {
        case RemoteInterfaceType::DYNAMIC_ENTRY:
          return "DYNAMIC";
        case RemoteInterfaceType::STATIC_ENTRY:
          return "STATIC";
      }
    }
    return "--";
  };

  auto getRemoteIntfLivenessStatusStr =
      [](const auto& remoteIntfLivenessStatus) {
        if (remoteIntfLivenessStatus.has_value()) {
          switch (remoteIntfLivenessStatus.value()) {
            case LivenessStatus::LIVE:
              return "LIVE";
            case LivenessStatus::STALE:
              return "STALE";
          }
        }
        return "--";
      };

  for (const auto& [rifId, rif] : rifs) {
    cli::RifEntry rifEntry;

    rifEntry.name() = *rif.interfaceName();
    rifEntry.rifID() = *rif.interfaceId();
    rifEntry.osIfName() = fmt::format("fboss{}", *rif.interfaceId());
    if (*rif.vlanId() != ctrl_constants::NO_VLAN()) {
      rifEntry.vlanID() = *rif.vlanId();
    }
    rifEntry.portNames() = *rif.portNames();
    rifEntry.routerID() = *rif.routerId();
    rifEntry.mac() = *rif.mac();
    rifEntry.mtu() = *rif.mtu();
    rifEntry.remoteInterfaceType() = getRemoteIntfTypeStr(rif.remoteIntfType());
    rifEntry.remoteInterfaceLivenessStatus() =
        getRemoteIntfLivenessStatusStr(rif.remoteIntfLivenessStatus());
    rifEntry.scope() = apache::thrift::util::enumNameSafe(*rif.scope());

    // Populate addresses
    for (const auto& addr : *rif.address()) {
      auto ip = folly::IPAddress::fromBinary(
          folly::ByteRange(
              reinterpret_cast<const unsigned char*>(addr.ip()->addr()->data()),
              addr.ip()->addr()->size()));
      rifEntry.addrs()->push_back(
          folly::to<std::string>(ip.str(), "/", *addr.prefixLength()));
    }

    model.rifs()->push_back(rifEntry);
  }

  return model;
}

void CmdShowRif::printOutput(const RetType& model, std::ostream& out) {
  Table outTable;
  outTable.setHeader(
      {"RIF",
       "RIFID",
       "OS Intf",
       "VlanID",
       "RouterID",
       "MAC",
       "MTU",
       "TYPE",
       "Liveness",
       "Scope",
       "Ports",
       "Addresses"});

  for (const auto& rif : model.get_rifs()) {
    outTable.addRow({
        rif.get_name(),
        std::to_string(rif.get_rifID()),
        rif.get_osIfName(),
        (rif.vlanID() ? std::to_string(*rif.vlanID()) : "--"),
        std::to_string(rif.get_routerID()),
        rif.get_mac(),
        std::to_string(rif.get_mtu()),
        rif.get_remoteInterfaceType(),
        rif.get_remoteInterfaceLivenessStatus(),
        rif.get_scope(),
        (rif.portNames()->size() > 0 ? folly::join("\n", *rif.portNames())
                                     : ""),
        (rif.addrs()->size() > 0 ? folly::join("\n", *rif.addrs()) : ""),
    });
  }

  out << outTable << std::endl;
}

// Human description for auto-generated CLI reference wiki.
std::string_view CmdShowRifTraits::description() {
  return "Displays the switch's router interfaces (RIFs): each RIF's ID, OS interface, VLAN, router ID, MAC, MTU, type, liveness, scope, member ports, and configured IP addresses. Use it to inspect L3 interface configuration.";
}

// Synthetic populated model for CLI reference wiki documentation.
RetType CmdShowRif::sampleModel() {
  RetType model;

  cli::RifEntry entry1;
  entry1.name() = "Interface 10";
  entry1.rifID() = 10;
  entry1.osIfName() = "fboss10";
  entry1.vlanID() = 10;
  entry1.routerID() = 0;
  entry1.mac() = "02:00:11:22:33:01";
  entry1.mtu() = 9000;
  entry1.remoteInterfaceType() = "--";
  entry1.remoteInterfaceLivenessStatus() = "--";
  entry1.scope() = "LOCAL";
  entry1.portNames() = {};
  entry1.addrs() = {
      "2001:db8:a::/128",
      "fe80::200:11ff:fe22:3301/64",
  };
  model.rifs()->push_back(entry1);

  cli::RifEntry entry2;
  entry2.name() = "vlan2001";
  entry2.rifID() = 2001;
  entry2.osIfName() = "fboss2001";
  entry2.vlanID() = 2001;
  entry2.routerID() = 0;
  entry2.mac() = "02:00:11:22:33:01";
  entry2.mtu() = 9000;
  entry2.remoteInterfaceType() = "--";
  entry2.remoteInterfaceLivenessStatus() = "--";
  entry2.scope() = "LOCAL";
  entry2.portNames() = {"eth1/1/1"};
  entry2.addrs() = {
      "2001:db8:b::63/127",
      "fe80::200:11ff:fe22:3301/64",
  };
  model.rifs()->push_back(entry2);

  return model;
}

// Explicit template instantiation
template void CmdHandler<CmdShowRif, CmdShowRifTraits>::run();
template const ValidFilterMapType
CmdHandler<CmdShowRif, CmdShowRifTraits>::getValidFilters();

} // namespace facebook::fboss
