/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "CmdShowAcl.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "fboss/agent/AddressUtil.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

using RetType = CmdShowAclTraits::RetType;

RetType CmdShowAcl::queryClient(const HostInfo& hostInfo) {
  facebook::fboss::AclTableThrift entries;
  std::vector<facebook::fboss::AclEntryThrift> aclEntries;
  auto client =
      utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);
  client->sync_getAclTableGroup(entries);

  return createModel(entries);
}

void CmdShowAcl::printOutput(const RetType& model, std::ostream& out) {
  for (const auto& aclTable : model.aclTableEntries().value()) {
    out << "ACL Table Name: " << aclTable.first << std::endl;
    for (const auto& aclEntry : aclTable.second) {
      out << "Acl: " << aclEntry.name().value() << std::endl;
      out << "   priority: " << folly::copy(aclEntry.priority().value())
          << std::endl;
      if (aclEntry.srcIp().has_value()) {
        out << "   src ip: " << aclEntry.srcIp().value();
        if (aclEntry.srcIpPrefixLength().has_value()) {
          out << "/" << folly::copy(aclEntry.srcIpPrefixLength().value());
        }
        out << std::endl;
      }
      if (aclEntry.dstIp().has_value()) {
        out << "   dst ip: " << aclEntry.dstIp().value();
        if (aclEntry.dstIpPrefixLength().has_value()) {
          out << "/" << folly::copy(aclEntry.dstIpPrefixLength().value());
        }
        out << std::endl;
      }
      if (aclEntry.proto().has_value()) {
        out << "   proto: " << folly::copy(aclEntry.proto().value())
            << std::endl;
      }
      if (aclEntry.srcPort().has_value()) {
        out << "   src port: " << folly::copy(aclEntry.srcPort().value())
            << std::endl;
      }
      if (aclEntry.dstPort().has_value()) {
        out << "   dst port: " << folly::copy(aclEntry.dstPort().value())
            << std::endl;
      }
      if (aclEntry.ipFrag().has_value()) {
        out << "   ip fragment: " << folly::copy(aclEntry.ipFrag().value())
            << std::endl;
      }
      if (aclEntry.dscp().has_value()) {
        out << "   dscp: " << folly::copy(aclEntry.dscp().value()) << std::endl;
      }
      if (aclEntry.ipType().has_value()) {
        out << "   ip type: " << folly::copy(aclEntry.ipType().value())
            << std::endl;
      }
      if (aclEntry.icmpType().has_value()) {
        out << "   icmp type: " << folly::copy(aclEntry.icmpType().value())
            << std::endl;
      }
      if (aclEntry.icmpCode().has_value()) {
        out << "   icmp code: " << folly::copy(aclEntry.icmpCode().value())
            << std::endl;
      }
      if (aclEntry.ttl().has_value()) {
        out << "   ttl: " << folly::copy(aclEntry.ttl().value()) << std::endl;
      }
      if (aclEntry.l4SrcPort().has_value()) {
        out << "   L4 src port: " << folly::copy(aclEntry.l4SrcPort().value())
            << std::endl;
      }
      if (aclEntry.l4DstPort().has_value()) {
        out << "   L4 dst port: " << folly::copy(aclEntry.l4DstPort().value())
            << std::endl;
      }
      if (aclEntry.dstMac().has_value()) {
        out << "   dst mac: " << aclEntry.dstMac().value() << std::endl;
      }
      if (aclEntry.lookupClassL2().has_value()) {
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

RetType CmdShowAcl::createModel(facebook::fboss::AclTableThrift entries) {
  RetType model;
  std::map<std::string, std::vector<facebook::fboss::AclEntryThrift>>
      aclTableEntries = entries.aclTableEntries().value();

  for (auto& [aclTableName, aclEntries] : aclTableEntries) {
    for (const auto& entry : aclEntries) {
      cli::AclEntry aclDetails;

      aclDetails.name() = entry.name().value();
      aclDetails.priority() = folly::copy(entry.priority().value());
      // srcIp/dstIp are non-optional BinaryAddress on the wire; the agent
      // populates them from a default-constructed CIDRNetwork when unset,
      // yielding an empty addr and prefixLength=0. Treat prefixLength>0 as
      // the signal that the ACL actually qualifies on the address.
      if (folly::copy(entry.srcIpPrefixLength().value()) > 0 &&
          !entry.srcIp()->addr()->empty()) {
        aclDetails.srcIp() =
            facebook::network::toIPAddress(entry.srcIp().value()).str();
        aclDetails.srcIpPrefixLength() =
            folly::copy(entry.srcIpPrefixLength().value());
      }
      if (folly::copy(entry.dstIpPrefixLength().value()) > 0 &&
          !entry.dstIp()->addr()->empty()) {
        aclDetails.dstIp() =
            facebook::network::toIPAddress(entry.dstIp().value()).str();
        aclDetails.dstIpPrefixLength() =
            folly::copy(entry.dstIpPrefixLength().value());
      }
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
        aclDetails.ipFrag() =
            static_cast<int16_t>(*apache::thrift::get_pointer(entry.ipFrag()));
      }
      if (apache::thrift::get_pointer(entry.dscp())) {
        aclDetails.dscp() =
            static_cast<int16_t>(*apache::thrift::get_pointer(entry.dscp()));
      }
      if (apache::thrift::get_pointer(entry.ipType())) {
        aclDetails.ipType() =
            static_cast<int16_t>(*apache::thrift::get_pointer(entry.ipType()));
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

// Explicit template instantiation
template void CmdHandler<CmdShowAcl, CmdShowAclTraits>::run();
template const ValidFilterMapType
CmdHandler<CmdShowAcl, CmdShowAclTraits>::getValidFilters();

} // namespace facebook::fboss
