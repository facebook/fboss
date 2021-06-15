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

namespace facebook::fboss {

struct CmdShowAclTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::vector<facebook::fboss::AclEntryThrift>;
};

class CmdShowAcl : public CmdHandler<CmdShowAcl, CmdShowAclTraits> {
 public:
  using RetType = CmdShowAclTraits::RetType;

  RetType queryClient(const folly::IPAddress& hostIp) {
    RetType retVal;
    auto client = utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(
        hostIp.str());
    client->sync_getAclTable(retVal);

    return retVal;
  }

  void printOutput(const RetType& aclTable) {
    for (auto const& aclEntry : aclTable) {
      std::cout << "Acl: " << aclEntry.get_name() << std::endl;
      std::cout << "   priority: " << aclEntry.get_priority() << std::endl;
      if (aclEntry.get_proto()) {
        std::cout << "   proto: " << *aclEntry.get_proto() << std::endl;
      }
      if (aclEntry.get_srcPort()) {
        std::cout << "   src port: " << *aclEntry.get_srcPort() << std::endl;
      }
      if (aclEntry.get_dstPort()) {
        std::cout << "   dst port: " << *aclEntry.get_dstPort() << std::endl;
      }
      if (aclEntry.get_ipFrag()) {
        std::cout << "   ip fragment: " << *aclEntry.get_ipFrag() << std::endl;
      }
      if (aclEntry.get_dscp()) {
        std::cout << "   dscp: " << *aclEntry.get_dscp() << std::endl;
      }
      if (aclEntry.get_ipType()) {
        std::cout << "   ip type: " << *aclEntry.get_ipType() << std::endl;
      }
      if (aclEntry.get_icmpType()) {
        std::cout << "   icmp type: " << *aclEntry.get_icmpType() << std::endl;
      }
      if (aclEntry.get_icmpCode()) {
        std::cout << "   icmp code: " << *aclEntry.get_icmpCode() << std::endl;
      }
      if (aclEntry.get_ttl()) {
        std::cout << "   ttl: " << *aclEntry.get_ttl() << std::endl;
      }
      if (aclEntry.get_l4SrcPort()) {
        std::cout << "   L4 src port: " << *aclEntry.get_l4SrcPort()
                  << std::endl;
      }
      if (aclEntry.get_l4DstPort()) {
        std::cout << "   L4 dst port: " << *aclEntry.get_l4DstPort()
                  << std::endl;
      }
      if (aclEntry.get_dstMac()) {
        std::cout << "   dst mac: " << *aclEntry.get_dstMac() << std::endl;
      }
      if (aclEntry.get_lookupClassL2()) {
        std::cout << "   lookup class L2: " << *aclEntry.get_lookupClassL2()
                  << std::endl;
      }
      std::cout << "   action: " << aclEntry.get_actionType() << std::endl;
      std::cout << std::endl;
    }
  }
};

} // namespace facebook::fboss
