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
    for (const auto& aclTable : model.aclTableEntries().value()) {
      out << "ACL Table Name: " << aclTable.first << std::endl;
      for (const auto& aclEntry : aclTable.second) {
        out << "Acl: " << aclEntry.name().value() << std::endl;
        out << "   priority: " << folly::copy(aclEntry.priority().value())
            << std::endl;
        if (folly::copy(aclEntry.proto().value())) {
          out << "   proto: " << folly::copy(aclEntry.proto().value())
              << std::endl;
        }
        if (folly::copy(aclEntry.srcPort().value())) {
          out << "   src port: " << folly::copy(aclEntry.srcPort().value())
              << std::endl;
        }
        if (folly::copy(aclEntry.dstPort().value())) {
          out << "   dst port: " << folly::copy(aclEntry.dstPort().value())
              << std::endl;
        }
        if (folly::copy(aclEntry.ipFrag().value())) {
          out << "   ip fragment: " << folly::copy(aclEntry.ipFrag().value())
              << std::endl;
        }
        if (folly::copy(aclEntry.dscp().value())) {
          out << "   dscp: " << folly::copy(aclEntry.dscp().value())
              << std::endl;
        }
        if (folly::copy(aclEntry.ipType().value())) {
          out << "   ip type: " << folly::copy(aclEntry.ipType().value())
              << std::endl;
        }
        if (folly::copy(aclEntry.icmpType().value())) {
          out << "   icmp type: " << folly::copy(aclEntry.icmpType().value())
              << std::endl;
        }
        if (folly::copy(aclEntry.icmpCode().value())) {
          out << "   icmp code: " << folly::copy(aclEntry.icmpCode().value())
              << std::endl;
        }
        if (folly::copy(aclEntry.ttl().value())) {
          out << "   ttl: " << folly::copy(aclEntry.ttl().value()) << std::endl;
        }
        if (folly::copy(aclEntry.l4SrcPort().value())) {
          out << "   L4 src port: " << folly::copy(aclEntry.l4SrcPort().value())
              << std::endl;
        }
        if (folly::copy(aclEntry.l4DstPort().value())) {
          out << "   L4 dst port: " << folly::copy(aclEntry.l4DstPort().value())
              << std::endl;
        }
        if (aclEntry.dstMac().value() != "") {
          out << "   dst mac: " << aclEntry.dstMac().value() << std::endl;
        }
        if (folly::copy(aclEntry.lookupClassL2().value())) {
          out << "   lookup class L2: "
              << folly::copy(aclEntry.lookupClassL2().value()) << std::endl;
        }
        out << "   action: " << aclEntry.actionType().value() << std::endl;
        if (folly::copy(aclEntry.enabled().value())) {
          out << "   enabled: " << folly::copy(aclEntry.enabled().value())
              << std::endl;
        }
        out << std::endl;
      }
    }
  }

  RetType createModel(facebook::fboss::AclTableThrift entries) {
    RetType model;
    std::map<std::string, std::vector<facebook::fboss::AclEntryThrift>>
        aclTableEntries = entries.aclTableEntries().value();

    for (auto& [aclTableName, aclEntries] : aclTableEntries) {
      for (const auto& entry : aclEntries) {
        cli::AclEntry aclDetails;

        aclDetails.name() = entry.name().value();
        aclDetails.priority() = folly::copy(entry.priority().value());
        if (apache::thrift::get_pointer(entry.proto())) {
          aclDetails.proto() =
              static_cast<int16_t>(*apache::thrift::get_pointer(entry.proto()));
        }
        if (apache::thrift::get_pointer(entry.srcPort())) {
          aclDetails.srcPort() = *apache::thrift::get_pointer(entry.srcPort());
        }
        if (apache::thrift::get_pointer(entry.dstPort())) {
          aclDetails.dstPort() = *apache::thrift::get_pointer(entry.dstPort());
        }
        if (apache::thrift::get_pointer(entry.ipFrag())) {
          aclDetails.ipFrag() = static_cast<int16_t>(
              *apache::thrift::get_pointer(entry.ipFrag()));
        }
        if (apache::thrift::get_pointer(entry.dscp())) {
          aclDetails.dscp() =
              static_cast<int16_t>(*apache::thrift::get_pointer(entry.dscp()));
        }
        if (apache::thrift::get_pointer(entry.ipType())) {
          aclDetails.ipType() = static_cast<int16_t>(
              *apache::thrift::get_pointer(entry.ipType()));
        }
        if (apache::thrift::get_pointer(entry.icmpType())) {
          aclDetails.icmpType() = static_cast<int16_t>(
              *apache::thrift::get_pointer(entry.icmpType()));
        }
        if (apache::thrift::get_pointer(entry.icmpCode())) {
          aclDetails.icmpCode() = static_cast<int16_t>(
              *apache::thrift::get_pointer(entry.icmpCode()));
        }
        if (apache::thrift::get_pointer(entry.ttl())) {
          aclDetails.ttl() = *apache::thrift::get_pointer(entry.ttl());
        }
        if (apache::thrift::get_pointer(entry.l4SrcPort())) {
          aclDetails.l4SrcPort() =
              *apache::thrift::get_pointer(entry.l4SrcPort());
        }
        if (apache::thrift::get_pointer(entry.l4DstPort())) {
          aclDetails.l4DstPort() =
              *apache::thrift::get_pointer(entry.l4DstPort());
        }
        if (apache::thrift::get_pointer(entry.dstMac())) {
          aclDetails.dstMac() = *apache::thrift::get_pointer(entry.dstMac());
        }
        if (apache::thrift::get_pointer(entry.lookupClassL2())) {
          aclDetails.lookupClassL2() = static_cast<int16_t>(
              *apache::thrift::get_pointer(entry.lookupClassL2()));
        }
        aclDetails.actionType() = entry.actionType().value();
        if (apache::thrift::get_pointer(entry.enabled())) {
          aclDetails.enabled() = *apache::thrift::get_pointer(entry.enabled());
        }
        model.aclTableEntries()[aclTableName].push_back(aclDetails);
      }
    }
    return model;
  }
};

} // namespace facebook::fboss
