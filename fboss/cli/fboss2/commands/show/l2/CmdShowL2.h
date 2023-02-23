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

#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/l2/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

using utils::Table;

struct CmdShowL2Traits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowL2Model;
};

class CmdShowL2 : public CmdHandler<CmdShowL2, CmdShowL2Traits> {
 public:
  using RetType = CmdShowL2Traits::RetType;

  RetType queryClient(const HostInfo& hostInfo) {
    std::vector<facebook::fboss::L2EntryThrift> entries;
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    client->sync_getL2Table(entries);
    return createModel(entries);
  }

  RetType createModel(
      const std::vector<facebook::fboss::L2EntryThrift>& l2Entries) {
    RetType model;
    for (const auto& entry : l2Entries) {
      cli::ShowL2ModelEntry l2Details;
      l2Details.mac() = entry.get_mac();
      l2Details.port() = entry.get_port();
      l2Details.vlanID() = entry.get_vlanID();
      if (auto trunk = entry.get_trunk()) {
        l2Details.trunk() = folly::to<std::string>(*trunk);
      } else {
        l2Details.trunk() = "-";
      }
      l2Details.type() = getTypeStr(entry.get_l2EntryType());
      if (auto classID = entry.get_classID()) {
        l2Details.classID() = folly::to<std::string>(*classID);
      } else {
        l2Details.classID() = "-";
      }
      model.l2Entries()->push_back(l2Details);
    }
    return model;
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;
    table.setHeader(
        {"MAC Address", "Port", "Trunk", "VLAN", "Type", "Class ID"});
    for (const auto& l2Entry : model.get_l2Entries()) {
      table.addRow(
          {l2Entry.get_mac(),
           folly::to<std::string>(l2Entry.get_port()),
           folly::to<std::string>(l2Entry.get_trunk()),
           folly::to<std::string>(l2Entry.get_vlanID()),
           l2Entry.get_type(),
           folly::to<std::string>(l2Entry.get_classID())});
    }
    out << table << std::endl;
  }

  std::string getTypeStr(L2EntryType type) {
    switch (type) {
      case L2EntryType::L2_ENTRY_TYPE_PENDING:
        return "PENDING";
      case L2EntryType::L2_ENTRY_TYPE_VALIDATED:
        return "VALIDATED";
    }

    throw std::runtime_error(
        "Unsupported L2EntryType: " + std::to_string(static_cast<int>(type)));
  }
};

} // namespace facebook::fboss
