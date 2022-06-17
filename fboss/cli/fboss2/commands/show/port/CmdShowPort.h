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

#include "fboss/cli/fboss2/CmdHandler.h"

#include <re2/re2.h>
#include <thrift/lib/cpp/transport/TTransportException.h>
#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/commands/show/port/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <unistd.h>
#include <algorithm>

namespace facebook::fboss {

using utils::Table;

struct CmdShowPortTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = cli::ShowPortModel;
  static constexpr std::array<std::string_view, 1> FILTERS = {"LinkState"};
};

class CmdShowPort : public CmdHandler<CmdShowPort, CmdShowPortTraits> {
 public:
  using ObjectArgType = CmdShowPortTraits::ObjectArgType;
  using RetType = CmdShowPortTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedPorts) {
    std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
    std::map<int32_t, facebook::fboss::TransceiverInfo> transceiverEntries;
    std::vector<int32_t> requiredTransceiverEntries;

    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);
    client->sync_getAllPortInfo(portEntries);

    try {
      auto qsfpService =
          utils::createClient<facebook::fboss::QsfpServiceAsyncClient>(
              hostInfo);

      qsfpService->sync_getTransceiverInfo(
          transceiverEntries, requiredTransceiverEntries);
    } catch (apache::thrift::transport::TTransportException& e) {
      std::cerr << "Cannot connect to qsfp_service\n";
    }

    return createModel(portEntries, transceiverEntries, queriedPorts);
  }

  const std::unordered_map<
      std::string_view,
      std::shared_ptr<CmdGlobalOptions::BaseTypeVerifier>>
  getValidFilters() {
    return {
        {"linkState",
         std::make_shared<CmdGlobalOptions::TypeVerifier<std::string>>(
             "linkState", std::vector<std::string>{"Up", "Down"})},
        {"adminState",
         std::make_shared<CmdGlobalOptions::TypeVerifier<std::string>>(
             "adminState", std::vector<std::string>{"Enabled", "Disabled"})},
        {"id", std::make_shared<CmdGlobalOptions::TypeVerifier<int>>("id")}};
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;
    table.setHeader(
        {"ID",
         "Name",
         "AdminState",
         "LinkState",
         "Transceiver",
         "TcvrID",
         "Speed",
         "ProfileID"});

    for (auto const& portInfo : model.get_portEntries()) {
      table.addRow({
          folly::to<std::string>(portInfo.get_id()),
          portInfo.get_name(),
          portInfo.get_adminState(),
          getStyledLinkState(portInfo.get_linkState()),
          portInfo.get_tcvrPresent(),
          folly::to<std::string>(portInfo.get_tcvrID()),
          portInfo.get_speed(),
          portInfo.get_profileId(),
      });
    }

    out << table << std::endl;
  }

  std::string getAdminStateStr(PortAdminState adminState) {
    switch (adminState) {
      case PortAdminState::DISABLED:
        return "Disabled";
      case PortAdminState::ENABLED:
        return "Enabled";
    }

    throw std::runtime_error(
        "Unsupported AdminState: " +
        std::to_string(static_cast<int>(adminState)));
  }

  Table::StyledCell getStyledLinkState(std::string linkState) {
    if (linkState == "Down") {
      return Table::StyledCell("Down", Table::Style::ERROR);
    } else {
      return Table::StyledCell("Up", Table::Style::GOOD);
    }

    throw std::runtime_error("Unsupported LinkState: " + linkState);
  }

  std::string getOperStateStr(PortOperState operState) {
    switch (operState) {
      case PortOperState::DOWN:
        return "Down";
      case PortOperState::UP:
        return "Up";
    }

    throw std::runtime_error(
        "Unsupported LinkState: " +
        std::to_string(static_cast<int>(operState)));
  }

  std::string getSpeedGbps(int64_t speedMbps) {
    return std::to_string(speedMbps / 1000) + "G";
  }

  /**
   * Whether the first port name is smaller than or equal to the second port
   * name. For example, eth1/5/3 will be parsed to four parts: eth(module name),
   * 1(module number), 5(port number), 3(subport number). The two number will
   * first be compared by the alphabetical order of their module names. If the
   * module namea are the same, then the order will be decied by the numerical
   * values of their module number, port number, subport number in order.
   * Therefore, eth420/5/1 comes before fab3/9/1 as the module name of the
   * former one is alphabetically smaller. However, eth420/5/1 comes after
   * eth1/5/3 as the module number 420 is larger than 1. Accordingly, eth1/5/3
   * comes after eth1/5/1 as its subport is larger. With input array
   * ["eth1/5/1", "eth1/5/2", "eth1/5/3", "fab402/20/1", "eth1/10/1",
   * "eth1/4/1"], the expected returned order of this comparator will be
   * ["eth1/4/1", "eth1/5/1", "eth1/5/2", "eth1/5/3",  "eth1/10/1",
   * "fab402/20/1"].
   * @param nameA    The first port name
   * @param nameB    The second port name
   * @return         true if the first port name is smaller or equal to the
   * second port name
   */
  bool compareByName(
      const std::basic_string<char>& nameA,
      const std::basic_string<char>& nameB) {
    static const RE2 exp("([a-z][a-z][a-z])(\\d+)/(\\d+)/(\\d)");
    std::string moduleNameA, moduleNumStrA, portNumStrA, subportNumStrA;
    std::string moduleNameB, moduleNumStrB, portNumStrB, subportNumStrB;
    if (!RE2::FullMatch(
            nameA,
            exp,
            &moduleNameA,
            &moduleNumStrA,
            &portNumStrA,
            &subportNumStrA)) {
      throw std::invalid_argument(folly::to<std::string>(
          "Invalid port name: ",
          nameA,
          "\nPort name must match 'moduleNum/port/subport' pattern"));
    }

    if (!RE2::FullMatch(
            nameB,
            exp,
            &moduleNameB,
            &moduleNumStrB,
            &portNumStrB,
            &subportNumStrB)) {
      throw std::invalid_argument(folly::to<std::string>(
          "Invalid port name: ",
          nameB,
          "\nPort name must match 'moduleNum/port/subport' pattern"));
    }

    int ret;
    if ((ret = moduleNameA.compare(moduleNameB)) != 0) {
      return ret < 0;
    }

    if (moduleNumStrA.compare(moduleNumStrB) != 0) {
      return stoi(moduleNumStrA) < stoi(moduleNumStrB);
    }

    if (portNumStrA.compare(portNumStrB) != 0) {
      return stoi(portNumStrA) < stoi(portNumStrB);
    }

    return stoi(subportNumStrA) < stoi(subportNumStrB);
  }

  std::string getTransceiverStr(
      std::map<int32_t, facebook::fboss::TransceiverInfo>& transceiverEntries,
      int32_t transceiverId) {
    if (transceiverEntries.count(transceiverId) == 0) {
      return "";
    }
    auto isPresent = transceiverEntries[transceiverId].get_present();
    if (isPresent)
      return "Present";
    return "Absent";
  }

  RetType createModel(
      std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries,
      std::map<int32_t, facebook::fboss::TransceiverInfo> transceiverEntries,
      const ObjectArgType& queriedPorts) {
    RetType model;
    std::unordered_set<std::string> queriedSet(
        queriedPorts.begin(), queriedPorts.end());

    for (const auto& entry : portEntries) {
      auto portInfo = entry.second;
      auto portName = portInfo.get_name();

      auto operState = getOperStateStr(portInfo.get_operState());

      if (queriedPorts.size() == 0 || queriedSet.count(portName)) {
        cli::PortEntry portDetails;
        portDetails.id() = portInfo.get_portId();
        portDetails.name() = portInfo.get_name();
        portDetails.adminState() = getAdminStateStr(portInfo.get_adminState());
        portDetails.linkState() = operState;
        portDetails.speed() = getSpeedGbps(portInfo.get_speedMbps());
        portDetails.profileId() = portInfo.get_profileID();
        if (auto tcvrId = portInfo.transceiverIdx()) {
          const auto transceiverId = tcvrId->get_transceiverId();
          portDetails.tcvrID() = transceiverId;
          portDetails.tcvrPresent() =
              getTransceiverStr(transceiverEntries, transceiverId);
        }
        model.portEntries()->push_back(portDetails);
      }
    }

    std::sort(
        model.portEntries()->begin(),
        model.portEntries()->end(),
        [&](const cli::PortEntry& a, const cli::PortEntry& b) {
          return compareByName(a.get_name(), b.get_name());
        });

    return model;
  }
};

} // namespace facebook::fboss
