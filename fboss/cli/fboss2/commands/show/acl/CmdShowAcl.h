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
#include "fboss/cli/fboss2/commands/show/acl/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

namespace facebook::fboss {

struct CmdShowAclTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowAclModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowAcl : public CmdHandler<CmdShowAcl, CmdShowAclTraits> {
 public:
  using RetType = CmdShowAclTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo) {
    facebook::fboss::AclTableThrift entries;
    std::vector<facebook::fboss::AclEntryThrift> aclEntries;
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);
    client->sync_getAclTableGroup(entries);

    return createModel(entries);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    for (const auto& aclTable : model.get_aclTableEntries()) {
      out << "ACL Table Name: " << aclTable.first << std::endl;
      for (const auto& aclEntry : aclTable.second) {
        out << "Acl: " << aclEntry.get_name() << std::endl;
        out << "   priority: " << aclEntry.get_priority() << std::endl;
        if (aclEntry.get_proto()) {
          out << "   proto: " << aclEntry.get_proto() << std::endl;
        }
        if (aclEntry.get_srcPort()) {
          out << "   src port: " << aclEntry.get_srcPort() << std::endl;
        }
        if (aclEntry.get_dstPort()) {
          out << "   dst port: " << aclEntry.get_dstPort() << std::endl;
        }
        if (aclEntry.get_ipFrag()) {
          out << "   ip fragment: " << aclEntry.get_ipFrag() << std::endl;
        }
        if (aclEntry.get_dscp()) {
          out << "   dscp: " << aclEntry.get_dscp() << std::endl;
        }
        if (aclEntry.get_ipType()) {
          out << "   ip type: " << aclEntry.get_ipType() << std::endl;
        }
        if (aclEntry.get_icmpType()) {
          out << "   icmp type: " << aclEntry.get_icmpType() << std::endl;
        }
        if (aclEntry.get_icmpCode()) {
          out << "   icmp code: " << aclEntry.get_icmpCode() << std::endl;
        }
        if (aclEntry.get_ttl()) {
          out << "   ttl: " << aclEntry.get_ttl() << std::endl;
        }
        if (aclEntry.get_l4SrcPort()) {
          out << "   L4 src port: " << aclEntry.get_l4SrcPort() << std::endl;
        }
        if (aclEntry.get_l4DstPort()) {
          out << "   L4 dst port: " << aclEntry.get_l4DstPort() << std::endl;
        }
        if (aclEntry.get_dstMac() != "") {
          out << "   dst mac: " << aclEntry.get_dstMac() << std::endl;
        }
        if (aclEntry.get_lookupClassL2()) {
          out << "   lookup class L2: " << aclEntry.get_lookupClassL2()
              << std::endl;
        }
        out << "   action: " << aclEntry.get_actionType() << std::endl;
        if (aclEntry.get_enabled()) {
          out << "   enabled: " << aclEntry.get_enabled() << std::endl;
        }
        out << std::endl;
      }
    }
  }

  RetType createModel(facebook::fboss::AclTableThrift entries) {
    RetType model;
    std::map<std::string, std::vector<facebook::fboss::AclEntryThrift>>
        aclTableEntries = entries.get_aclTableEntries();

    for (auto& [aclTableName, aclEntries] : aclTableEntries) {
      for (const auto& entry : aclEntries) {
        cli::AclEntry aclDetails;

        aclDetails.name() = entry.get_name();
        aclDetails.priority() = entry.get_priority();
        if (entry.get_proto()) {
          aclDetails.proto() = static_cast<int16_t>(*entry.get_proto());
        }
        if (entry.get_srcPort()) {
          aclDetails.srcPort() = *entry.get_srcPort();
        }
        if (entry.get_dstPort()) {
          aclDetails.dstPort() = *entry.get_dstPort();
        }
        if (entry.get_ipFrag()) {
          aclDetails.ipFrag() = static_cast<int16_t>(*entry.get_ipFrag());
        }
        if (entry.get_dscp()) {
          aclDetails.dscp() = static_cast<int16_t>(*entry.get_dscp());
        }
        if (entry.get_ipType()) {
          aclDetails.ipType() = static_cast<int16_t>(*entry.get_ipType());
        }
        if (entry.get_icmpType()) {
          aclDetails.icmpType() = static_cast<int16_t>(*entry.get_icmpType());
        }
        if (entry.get_icmpCode()) {
          aclDetails.icmpCode() = static_cast<int16_t>(*entry.get_icmpCode());
        }
        if (entry.get_ttl()) {
          aclDetails.ttl() = *entry.get_ttl();
        }
        if (entry.get_l4SrcPort()) {
          aclDetails.l4SrcPort() = *entry.get_l4SrcPort();
        }
        if (entry.get_l4DstPort()) {
          aclDetails.l4DstPort() = *entry.get_l4DstPort();
        }
        if (entry.get_dstMac()) {
          aclDetails.dstMac() = *entry.get_dstMac();
        }
        if (entry.get_lookupClassL2()) {
          aclDetails.lookupClassL2() =
              static_cast<int16_t>(*entry.get_lookupClassL2());
        }
        aclDetails.actionType() = entry.get_actionType();
        if (entry.get_enabled()) {
          aclDetails.enabled() = *entry.get_enabled();
        }
        model.aclTableEntries()[aclTableName].push_back(aclDetails);
      }
    }
    return model;
  }
};

} // namespace facebook::fboss
