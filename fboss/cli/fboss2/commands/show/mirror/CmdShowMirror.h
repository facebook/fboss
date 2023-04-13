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

#include "fboss/agent/AddressUtil.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/mirror/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

#include "folly/json.h"

namespace facebook::fboss {

using utils::Table;

struct CmdShowMirrorTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_MIRROR_LIST;
  using ObjectArgType = utils::MirrorList;
  using RetType = cli::ShowMirrorModel;
};

class CmdShowMirror : public CmdHandler<CmdShowMirror, CmdShowMirrorTraits> {
 public:
  using ObjectArgType = CmdShowMirrorTraits::ObjectArgType;
  using RetType = CmdShowMirrorTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedMirrors) {
    std::string mirrorMaps;
    std::map<int32_t, PortInfoThrift> portInfoEntries;
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    client->sync_getCurrentStateJSON(mirrorMaps, "mirrorMaps");
    client->sync_getAllPortInfo(portInfoEntries);
    return createModel(mirrorMaps, portInfoEntries, queriedMirrors);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;
    table.setHeader(
        {"Mirror",
         "Status",
         "Egress Port",
         "Egress Port Name",
         "Tunnel Type",
         "Src MAC",
         "Src IP",
         "Src UDP Port",
         "Dst MAC",
         "Dst IP",
         "Dst UDP Port",
         "DSCP"});
    for (const auto& mirrorEntry : model.get_mirrorEntries()) {
      table.addRow(
          {mirrorEntry.get_mirror(),
           mirrorEntry.get_status(),
           mirrorEntry.get_egressPort(),
           mirrorEntry.get_egressPortName(),
           mirrorEntry.get_mirrorTunnelType(),
           mirrorEntry.get_srcMAC(),
           mirrorEntry.get_srcIP(),
           mirrorEntry.get_srcUDPPort(),
           mirrorEntry.get_dstMAC(),
           mirrorEntry.get_dstIP(),
           mirrorEntry.get_dstUDPPort(),
           mirrorEntry.get_dscp()});
    }
    out << table << std::endl;
  }

  std::string getEgressPortName(
      const std::string& egressPort,
      const std::map<int32_t, PortInfoThrift>& portInfoEntries) {
    std::int32_t egressPortID = folly::to<int>(egressPort);
    auto egressPortInfoEntry = portInfoEntries.find(egressPortID);
    if (egressPortInfoEntry != portInfoEntries.end()) {
      const auto& egressPortInfo = egressPortInfoEntry->second;
      return egressPortInfo.get_name();
    }
    return "Unknown";
  }

  std::string getIPAddressStr(const std::string& rawIPAddress) {
    auto thriftIPAddress = apache::thrift::SimpleJSONSerializer::deserialize<
        network::thrift::BinaryAddress>(rawIPAddress);
    return network::toIPAddress(thriftIPAddress).str();
  }

  void processTunnel(
      const folly::dynamic& tunnel,
      cli::ShowMirrorModelEntry& mirrorDetails) {
    mirrorDetails.srcMAC() = tunnel["srcMac"].asString();
    mirrorDetails.srcIP() = getIPAddressStr(folly::toJson(tunnel["srcIp"]));
    mirrorDetails.dstMAC() = tunnel["dstMac"].asString();
    mirrorDetails.dstIP() = getIPAddressStr(folly::toJson(tunnel["dstIp"]));
    bool isSFlow = true;
    auto srcUDPPort = tunnel.find("udpSrcPort");
    if (srcUDPPort != tunnel.items().end()) {
      mirrorDetails.srcUDPPort() = srcUDPPort->second.asString();
    } else {
      mirrorDetails.srcUDPPort() = "-";
      isSFlow = false;
    }
    auto dstUDPPort = tunnel.find("udpDstPort");
    if (dstUDPPort != tunnel.items().end()) {
      mirrorDetails.dstUDPPort() = dstUDPPort->second.asString();
    } else {
      mirrorDetails.dstUDPPort() = "-";
      isSFlow = false;
    }
    mirrorDetails.mirrorTunnelType() = isSFlow ? "sFlow" : "GRE";
  }

  void processWithNoTunnel(
      const folly::dynamic& mirrorMapEntry,
      cli::ShowMirrorModelEntry& mirrorDetails) {
    mirrorDetails.mirrorTunnelType() = "-";
    mirrorDetails.srcMAC() = "-";
    auto srcIP = mirrorMapEntry.find("srcIp");
    if (srcIP != mirrorMapEntry.items().end()) {
      mirrorDetails.srcIP() = getIPAddressStr(folly::toJson(srcIP->second));
    } else {
      mirrorDetails.srcIP() = "-";
    }
    auto srcUDPPort = mirrorMapEntry.find("udpSrcPort");
    if (srcUDPPort != mirrorMapEntry.items().end()) {
      mirrorDetails.srcUDPPort() = srcUDPPort->second.asString();
    } else {
      mirrorDetails.srcUDPPort() = "-";
    }
    mirrorDetails.dstMAC() = "-";
    auto dstIP = mirrorMapEntry.find("destinationIp");
    if (dstIP != mirrorMapEntry.items().end()) {
      mirrorDetails.dstIP() = getIPAddressStr(folly::toJson(dstIP->second));
    } else {
      mirrorDetails.dstIP() = "-";
    }
    auto dstUDPPort = mirrorMapEntry.find("udpDstPort");
    if (dstUDPPort != mirrorMapEntry.items().end()) {
      mirrorDetails.dstUDPPort() = dstUDPPort->second.asString();
    } else {
      mirrorDetails.dstUDPPort() = "-";
    }
  }

  RetType createModel(
      const std::string& mirrorMaps,
      const std::map<int32_t, PortInfoThrift>& portInfoEntries,
      const ObjectArgType& queriedMirrors) {
    RetType model;
    std::unordered_set<std::string> queriedSet(
        queriedMirrors.begin(), queriedMirrors.end());
    auto mirrorMapsEntries = folly::parseJson(mirrorMaps);
    // TODO: Handle NPU ID for Multi-NPU Cases
    auto mirrorMapEntries = mirrorMapsEntries["id=0"];
    for (const auto& mirrorMapEntryItem : mirrorMapEntries.items()) {
      cli::ShowMirrorModelEntry mirrorDetails;
      const auto& mirrorMapEntry = mirrorMapEntryItem.second;
      auto mirrorName = mirrorMapEntry["name"].asString();
      if (queriedSet.size() > 0 && queriedSet.count(mirrorName) == 0) {
        continue;
      }
      mirrorDetails.mirror() = mirrorMapEntry["name"].asString();
      mirrorDetails.status() =
          (mirrorMapEntry["isResolved"].asBool()) ? "Active" : "Configured";
      auto egressPort = mirrorMapEntry.find("egressPort");
      if (egressPort != mirrorMapEntry.items().end()) {
        std::string egressPortID = egressPort->second.asString();
        mirrorDetails.egressPort() = egressPortID;
        mirrorDetails.egressPortName() =
            getEgressPortName(egressPortID, portInfoEntries);
      } else {
        mirrorDetails.egressPort() = "-";
        mirrorDetails.egressPortName() = "-";
      }
      auto tunnel = mirrorMapEntry.find("tunnel");
      if (tunnel != mirrorMapEntry.items().end()) {
        processTunnel(tunnel->second, mirrorDetails);
      } else {
        processWithNoTunnel(mirrorMapEntry, mirrorDetails);
      }
      mirrorDetails.dscp() = mirrorMapEntry["dscp"].asString();
      model.mirrorEntries()->push_back(mirrorDetails);
    }
    return model;
  }
};

} // namespace facebook::fboss
