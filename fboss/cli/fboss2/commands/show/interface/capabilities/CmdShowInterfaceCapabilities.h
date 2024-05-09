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
#include "fboss/cli/fboss2/commands/show/interface/CmdShowInterface.h"
#include "fboss/cli/fboss2/commands/show/interface/capabilities/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/Table.h"

#include <boost/algorithm/string.hpp>
#include <unordered_set>

namespace facebook::fboss {

using utils::Table;

struct CmdShowInterfaceCapabilitiesTraits : public BaseCommandTraits {
  using ParentCmd = CmdShowInterface;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::InterfaceCapabilitiesModel;
};

class CmdShowInterfaceCapabilities : public CmdHandler<
                                         CmdShowInterfaceCapabilities,
                                         CmdShowInterfaceCapabilitiesTraits> {
 public:
  using ObjectArgType = CmdShowInterfaceCapabilitiesTraits::ObjectArgType;
  using RetType = CmdShowInterfaceCapabilitiesTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs) {
    auto client =
        utils::createClient<facebook::fboss::QsfpServiceAsyncClient>(hostInfo);

    std::map<std::string, std::vector<cfg::PortProfileID>> portProfiles;
    client->sync_getAllPortSupportedProfiles(portProfiles, true);

    return createModel(portProfiles, queriedIfs);
  }

  RetType createModel(
      const std::map<std::string, std::vector<cfg::PortProfileID>>&
          portProfiles,
      const std::vector<std::string>& queriedIfs) {
    RetType model;

    if (queriedIfs.empty()) {
      model.portProfiles() = portProfiles;
    } else {
      for (auto& interface : queriedIfs) {
        if (portProfiles.find(interface) != portProfiles.end()) {
          model.portProfiles()[interface] = portProfiles.at(interface);
        }
      }
    }

    return model;
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;
    table.setHeader({"Interface", "Profiles"});

    for (auto& [portName, profileList] : model.portProfiles().value()) {
      std::string profileListStr;
      for (auto& profile : profileList) {
        profileListStr +=
            apache::thrift::util::enumNameSafe(profile) + "\n               ";
      }
      table.addRow({portName, profileListStr});
    }
    out << table << std::endl;
  }
};
} // namespace facebook::fboss
