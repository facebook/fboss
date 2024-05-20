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

#include <fboss/agent/if/gen-cpp2/ctrl_constants.h>
#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <cstdint>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/rif/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

namespace facebook::fboss {

struct CmdShowRifTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowRifModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowRif : public CmdHandler<CmdShowRif, CmdShowRifTraits> {
 public:
  using ObjectArgType = CmdShowRifTraits::ObjectArgType;
  using RetType = CmdShowRifTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo) {
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);

    std::map<int32_t, facebook::fboss::InterfaceDetail> rifs;
    client->sync_getAllInterfaces(rifs);

    return createModel(rifs);
  }

  RetType createModel(
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
      if (*rif.vlanId() != ctrl_constants::NO_VLAN()) {
        rifEntry.vlanID() = *rif.vlanId();
      }
      rifEntry.routerID() = *rif.routerId();
      rifEntry.mac() = *rif.mac();
      rifEntry.mtu() = *rif.mtu();
      rifEntry.remoteInterfaceType() =
          getRemoteIntfTypeStr(rif.remoteIntfType());
      rifEntry.remoteInterfaceLivenessStatus() =
          getRemoteIntfLivenessStatusStr(rif.remoteIntfLivenessStatus());
      rifEntry.scope() = apache::thrift::util::enumNameSafe(*rif.scope());

      model.rifs()->push_back(rifEntry);
    }

    return model;
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table outTable;
    outTable.setHeader(
        {"RIF",
         "RIFID",
         "VlanID",
         "RouterID",
         "MAC",
         "MTU",
         "TYPE",
         "Liveness",
         "Scope"});

    for (const auto& rif : model.get_rifs()) {
      outTable.addRow({
          rif.get_name(),
          std::to_string(rif.get_rifID()),
          (rif.vlanID() ? std::to_string(*rif.vlanID()) : "--"),
          std::to_string(rif.get_routerID()),
          rif.get_mac(),
          std::to_string(rif.get_mtu()),
          rif.get_remoteInterfaceType(),
          rif.get_remoteInterfaceLivenessStatus(),
          rif.get_scope(),
      });
    }

    out << outTable << std::endl;
  }
};

} // namespace facebook::fboss
