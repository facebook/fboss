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

#include "fboss/agent/if/gen-cpp2/FbossCtrlAsyncClient.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/transceiver/gen-cpp2/model_types.h"
#include "thrift/lib/cpp2/protocol/Serializer.h"

#include <fmt/core.h>

namespace facebook::fboss {

struct CmdShowTransceiverTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = cli::ShowTransceiverModel;
};

class CmdShowTransceiver
    : public CmdHandler<CmdShowTransceiver, CmdShowTransceiverTraits> {
 public:
  using ObjectArgType = CmdShowTransceiver::ObjectArgType;
  using RetType = CmdShowTransceiver::RetType;

  RetType queryClient(
      const folly::IPAddress& hostIp,
      const ObjectArgType& queriedPorts) {
    auto qsfpService =
        utils::createClient<QsfpServiceAsyncClient>(hostIp.str());
    auto agent = utils::createClient<FbossCtrlAsyncClient>(hostIp.str());

    // TODO: explore performance improvement if we make all this parallel.
    auto portEntries = queryPortInfo(agent.get(), queriedPorts);
    auto portStatusEntries = queryPortStatus(agent.get(), portEntries);
    auto transceiverEntries =
        queryTransceiverInfo(qsfpService.get(), portStatusEntries);

    return createModel(portStatusEntries, transceiverEntries);
  }

  void printOutput(const RetType& model) {
    std::string fmtString = "{:<8}{:<10}{:<15}{:<18}{:<20}{:<15}\n";
    std::cout << fmt::format(
        fmtString,
        "Port",
        "Status",
        "Present",
        "Vendor",
        "Serial",
        "Part Number");

    for (const auto& [portId, details] : model.get_transceivers()) {
      std::cout << fmt::format(
          fmtString,
          portId,
          details.get_isUp(),
          details.get_isPresent(),
          details.get_vendor(),
          details.get_serial(),
          details.get_partNumber());
    }
  }

 private:
  std::map<int, PortInfoThrift> queryPortInfo(
      FbossCtrlAsyncClient* agent,
      const ObjectArgType& queriedPorts) const {
    std::map<int, PortInfoThrift> portEntries;
    agent->sync_getAllPortInfo(portEntries);
    if (queriedPorts.size() == 0) {
      return portEntries;
    }

    std::map<int, PortInfoThrift> filteredPortEntries;
    for (auto const& [portId, portInfo] : portEntries) {
      for (auto const& queriedPort : queriedPorts) {
        if (portInfo.get_name() == queriedPort) {
          filteredPortEntries.emplace(portId, portInfo);
        }
      }
    }
    return filteredPortEntries;
  }

  std::map<int, PortStatus> queryPortStatus(
      FbossCtrlAsyncClient* agent,
      std::map<int, PortInfoThrift> portEntries) const {
    std::vector<int32_t> requiredPortStatus;
    for (const auto& portItr : portEntries) {
      requiredPortStatus.push_back(portItr.first);
    }
    std::map<int, PortStatus> portStatusEntries;
    agent->sync_getPortStatus(portStatusEntries, requiredPortStatus);

    return portStatusEntries;
  }

  std::map<int, TransceiverInfo> queryTransceiverInfo(
      QsfpServiceAsyncClient* qsfpService,
      std::map<int, PortStatus> portStatusEntries) const {
    std::vector<int32_t> requiredTransceiverEntries;
    for (const auto& portStatusItr : portStatusEntries) {
      if (auto tidx = portStatusItr.second.transceiverIdx_ref()) {
        requiredTransceiverEntries.push_back(tidx->get_transceiverId());
      }
    }

    std::map<int, TransceiverInfo> transceiverEntries;
    qsfpService->sync_getTransceiverInfo(
        transceiverEntries, requiredTransceiverEntries);
    return transceiverEntries;
  }

  RetType createModel(
      std::map<int, PortStatus> portStatusEntries,
      std::map<int, TransceiverInfo> transceiverEntries) const {
    RetType model;

    // TODO: sort here?
    for (const auto& [portId, portEntry] : portStatusEntries) {
      cli::TransceiverDetail details;
      const auto transceiverId =
          portEntry.transceiverIdx_ref()->get_transceiverId();
      const auto& transceiver = transceiverEntries[transceiverId];
      details.isUp_ref() = portEntry.get_up();
      details.isPresent_ref() = transceiver.get_present();
      if (const auto& vendor = transceiver.vendor_ref()) {
        details.vendor_ref() = vendor->get_name();
        details.serial_ref() = vendor->get_serialNumber();
        details.partNumber_ref() = vendor->get_partNumber();
      }
      model.transceivers_ref()->emplace(portId, std::move(details));
    }
    return model;
  }
};

} // namespace facebook::fboss
