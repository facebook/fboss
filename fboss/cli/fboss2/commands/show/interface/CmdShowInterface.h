/*
 *  Copyright (c) Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

#include <re2/re2.h>

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

std::string getRif(const std::string& name, int32_t port) {
  if (port != 0 && (name.rfind("eth", 0) == 0 || name.rfind("evt", 0) == 0)) {
    return "fboss" + std::to_string(port);
  }
  return "";
}

} // anonymous namespace

namespace facebook::fboss {

using utils::Table;

struct CmdShowInterfaceTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = cli::ShowInterfaceModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowInterface
    : public CmdHandler<CmdShowInterface, CmdShowInterfaceTraits> {
 private:
  bool isVoq = false;

 public:
  using ObjectArgType = CmdShowInterfaceTraits::ObjectArgType;
  using RetType = CmdShowInterfaceTraits::RetType;

  // PortPosition stores physical port location based on ethernet
  // port name, e.g. eth1/2/1 means ethernet port in linecard 1,
  // module 2, port 1. This will be used to sort the output.
  struct PortPosition {
    int32_t linecard;
    int32_t mod;
    int32_t port;
  };

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedIfs) {
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);

    std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
    std::map<int32_t, facebook::fboss::InterfaceDetail> intfDetails;

    client->sync_getAllPortInfo(portEntries);
    client->sync_getAllInterfaces(intfDetails);

    // Get DSF nodes data
    std::map<int64_t, cfg::DsfNode> dsfNodes;
    try {
      std::map<int64_t, cfg::SwitchInfo> switchIdToSwitchInfo;
      client->sync_getSwitchIdToSwitchInfo(switchIdToSwitchInfo);
      isVoq =
          (cfg::SwitchType::VOQ ==
           facebook::fboss::utils::getSwitchType(switchIdToSwitchInfo));
      if (isVoq) {
        client->sync_getDsfNodes(dsfNodes);
      }
    }
    // Thrift exception is expected to be thrown for switches which do not yet
    // have the API getSwitchIdToSwitchInfo implemented.
    catch (const std::exception& e) {
      XLOG(DBG2) << e.what();
    }

    return createModel(
        hostInfo, portEntries, intfDetails, dsfNodes, queriedIfs);
  }

  RetType createModel(
      const HostInfo& hostInfo,
      std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries,
      std::map<int32_t, facebook::fboss::InterfaceDetail> intfDetails,
      std::map<int64_t, cfg::DsfNode> dsfNodes,
      const ObjectArgType& queriedIfs) {
    RetType model;
    std::unordered_set<std::string> queriedSet(
        queriedIfs.begin(), queriedIfs.end());
    std::unordered_map<int32_t, int32_t> vlanToMtu;
    std::unordered_map<int32_t, std::vector<cli::IpPrefix>> vlanToPrefixes;

    populateVlanToMtu(vlanToMtu, intfDetails);
    populateVlanToPrefixes(vlanToPrefixes, intfDetails);

    int32_t minSystemPort = 0;
    for (const auto& idAndNode : dsfNodes) {
      const auto& node = idAndNode.second;
      if (hostInfo.getName() == *node.name()) {
        minSystemPort = *node.systemPortRange()->minimum();
      }
    }

    for (const auto& [portId, portInfo] : portEntries) {
      if (queriedIfs.size() == 0 || queriedSet.count(*portInfo.name())) {
        cli::Interface ifModel;
        const auto operState = *portInfo.operState();
        ifModel.name() = *portInfo.name();
        ifModel.description() = *portInfo.description();
        ifModel.status() =
            (operState == facebook::fboss::PortOperState::UP) ? "up" : "down";
        ifModel.speed() = std::to_string(*portInfo.speedMbps() / 1000) + "G";
        ifModel.prefixes() = {};
        ifModel.portType() = *portInfo.portType();

        // We assume that there is a one-to-one association between
        // port, interface, and VLAN.
        if (portInfo.vlans()->size() == 1) {
          const auto& vlan = portInfo.vlans()->at(0);
          ifModel.vlan() = vlan;
          if (vlanToMtu.contains(vlan)) {
            ifModel.mtu() = vlanToMtu[vlan];
          }
          if (vlanToPrefixes.contains(vlan)) {
            ifModel.prefixes() = vlanToPrefixes[vlan];
          }
        }

        if (dsfNodes.size() > 0) {
          int systemPortId = minSystemPort + portId;
          ifModel.systemPortId() = systemPortId;

          // Extract IP addresses for DSF switches
          auto intf = intfDetails.find(systemPortId);
          if (intf != intfDetails.end()) {
            for (auto ipPrefix : *intf->second.address()) {
              cli::IpPrefix prefix;
              prefix.ip() = folly::IPAddress::fromBinary(
                                folly::ByteRange(
                                    reinterpret_cast<const unsigned char*>(
                                        ipPrefix.ip()->addr()->data()),
                                    ipPrefix.ip()->addr()->size()))
                                .str();
              prefix.prefixLength() = *ipPrefix.prefixLength();
              ifModel.prefixes()->push_back(std::move(prefix));
            }
          }
        }

        model.interfaces()->push_back(std::move(ifModel));
      }
    }

    sortByInterfaceName(model);

    return model;
  }

  void populateVlanToMtu(
      std::unordered_map<int32_t, int32_t>& vlanToMtu,
      const std::map<int32_t, facebook::fboss::InterfaceDetail>& intfDetails) {
    for (const auto& [intfId, intfDetail] : intfDetails) {
      vlanToMtu[*intfDetail.vlanId()] = *intfDetail.mtu();
    }
  }

  void populateVlanToPrefixes(
      std::unordered_map<int32_t, std::vector<cli::IpPrefix>>& vlanToPrefixes,
      const std::map<int32_t, facebook::fboss::InterfaceDetail>& intfDetails) {
    for (const auto& [intfId, intfDetail] : intfDetails) {
      if (intfDetail.remoteIntfType() == RemoteInterfaceType::DYNAMIC_ENTRY) {
        continue; // Only search for local interfaces
      }

      const auto& vlan = *intfDetail.vlanId();
      for (const auto& ifAddr : *intfDetail.address()) {
        cli::IpPrefix prefix;
        prefix.ip() = folly::IPAddress::fromBinary(
                          folly::ByteRange(
                              reinterpret_cast<const unsigned char*>(
                                  ifAddr.ip()->addr()->data()),
                              ifAddr.ip()->addr()->size()))
                          .str();
        prefix.prefixLength() = *ifAddr.prefixLength();
        vlanToPrefixes[vlan].emplace_back(std::move(prefix));
      }
    }
  }

  void sortByInterfaceName(RetType& model) {
    std::unordered_map<std::string, PortPosition> nameToPortPosition;
    std::vector<std::string> portNames;
    for (const auto& intf : *model.interfaces()) {
      portNames.emplace_back(*intf.name());
    }
    populateNameToPortPositionMap(nameToPortPosition, portNames);

    std::sort(
        model.interfaces()->begin(),
        model.interfaces()->end(),
        [&nameToPortPosition](cli::Interface& a, cli::Interface b) {
          if (nameToPortPosition.contains(*a.name()) and
              nameToPortPosition.contains(*b.name())) {
            const auto& aPos = nameToPortPosition[*a.name()];
            const auto& bPos = nameToPortPosition[*b.name()];
            if (aPos.linecard != bPos.linecard) {
              return aPos.linecard < bPos.linecard;
            }
            if (aPos.mod != bPos.mod) {
              return aPos.mod < bPos.mod;
            }
            return aPos.port < bPos.port;
          }
          return *a.name() < *b.name();
        });
  }

  void populateNameToPortPositionMap(
      std::unordered_map<std::string, PortPosition>& nameToPortPosition,
      const std::vector<std::string>& names) {
    re2::RE2 ethPortMatcher("eth([0-9]+)/([0-9]+)/([0-9]+)");
    for (const auto& name : names) {
      std::string tLinecard, tMod, tPort;
      if (re2::RE2::FullMatch(
              name, ethPortMatcher, &tLinecard, &tMod, &tPort)) {
        nameToPortPosition[name] = PortPosition{
            folly::to<int32_t>(tLinecard),
            folly::to<int32_t>(tMod),
            folly::to<int32_t>(tPort)};
      }
    }
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table outTable{true};

    if (isVoq) {
      outTable.setHeader({
          "Interface",
          "RIF",
          "Status",
          "Speed",
          "VLAN",
          "MTU",
          "Addresses",
          "Description",
      });
    } else {
      outTable.setHeader({
          "Interface",
          "Status",
          "Speed",
          "VLAN",
          "MTU",
          "Addresses",
          "Description",
      });
    }

    for (const auto& interface : *model.interfaces()) {
      std::string name = *interface.name();
      std::vector<std::string> prefixes;

      if (interface.portType() == cfg::PortType::FABRIC_PORT ||
          !name.starts_with("fab")) { // Skip addresses for fabric ports
        for (const auto& prefix : *interface.prefixes()) {
          prefixes.push_back(
              fmt::format("{}/{}", *prefix.ip(), *prefix.prefixLength()));
        }
      }

      std::vector<Table::RowData> row;
      if (isVoq) {
        outTable.addRow(
            {name,
             getRif(name, *interface.systemPortId()),
             colorStatusCell(*interface.status()),
             *interface.speed(),
             (interface.vlan() ? std::to_string(*interface.vlan()) : ""),
             (interface.mtu() ? std::to_string(*interface.mtu()) : ""),
             (prefixes.size() > 0 ? folly::join("\n", prefixes) : ""),
             *interface.description()});
      } else {
        outTable.addRow(
            {name,
             colorStatusCell(*interface.status()),
             *interface.speed(),
             (interface.vlan() ? std::to_string(*interface.vlan()) : ""),
             (interface.mtu() ? std::to_string(*interface.mtu()) : ""),
             (prefixes.size() > 0 ? folly::join("\n", prefixes) : ""),
             *interface.description()});
      }
    }
    out << outTable << std::endl;
  }

  Table::StyledCell colorStatusCell(std::string status) {
    Table::Style cellStyle = Table::Style::NONE;
    if (status == "up") {
      cellStyle = Table::Style::GOOD;
    }
    return Table::StyledCell(status, cellStyle);
  }
};

} // namespace facebook::fboss
