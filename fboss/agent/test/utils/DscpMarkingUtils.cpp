/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/utils/DscpMarkingUtils.h"

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/TrafficPolicyTestUtils.h"

#include <string>

namespace facebook::fboss::utility {

// 53: DNS, 88: Kerberos, 123: NTP, 546/547: DHCPV6, 514: syslog, 636: LDAP

const std::vector<uint32_t>& kTcpPorts() {
  static const std::vector<uint32_t> tcpPorts = {53, 88, 123, 636};

  return tcpPorts;
}

const std::vector<uint32_t>& kUdpPorts() {
  static const std::vector<uint32_t> udpPorts = {53, 88, 123, 546, 547, 514};

  return udpPorts;
}

std::string getDscpAclTableName() {
  return "acl-table-dscp";
}

std::string kDscpCounterAclName() {
  return "dscp_counter_acl";
}

std::string kCounterName() {
  return "dscp_counter";
}

uint8_t kIcpDscp() {
  return utility::kOlympicQueueToDscp()
      .at(utility::getOlympicQueueId(utility::OlympicQueueType::ICP))
      .front();
}

uint8_t kNcnfDscp() {
  return utility::kOlympicV2QueueToDscp()
      .at(utility::getOlympicV2QueueId(utility::OlympicV2QueueType::NCNF))
      .front();
}

std::string
getDscpAclName(IP_PROTO proto, const std::string& direction, uint32_t port) {
  return folly::to<std::string>(
      "dscp-mark-for-proto-",
      static_cast<int>(proto),
      "-L4-",
      direction,
      "-port-",
      port);
}

std::string getDscpReclassificationAclName() {
  return "dscp_reclassification";
}

void addDscpMarkingAclsHelper(
    const HwAsic* hwAsic,
    cfg::SwitchConfig* config,
    IP_PROTO proto,
    const std::vector<uint32_t>& ports,
    bool isSai) {
  for (auto port : ports) {
    auto l4SrcPortAclName = getDscpAclName(proto, "src", port);
    utility::addL4SrcPortAclToCfg(
        hwAsic, config, l4SrcPortAclName, proto, port);
    utility::addSetDscpAndEgressQueueActionToCfg(
        hwAsic,
        config,
        l4SrcPortAclName,
        kIcpDscp(),
        utility::getOlympicQueueId(utility::OlympicQueueType::ICP),
        isSai);

    auto l4DstPortAclName = getDscpAclName(proto, "dst", port);
    utility::addL4DstPortAclToCfg(
        hwAsic, config, l4DstPortAclName, proto, port);
    utility::addSetDscpAndEgressQueueActionToCfg(
        hwAsic,
        config,
        l4DstPortAclName,
        kIcpDscp(),
        utility::getOlympicQueueId(utility::OlympicQueueType::ICP),
        isSai);
  }
}

void addDscpReclassificationAcls(
    const HwAsic* hwAsic,
    cfg::SwitchConfig* config,
    const PortID& portId) {
  auto acl = cfg::AclEntry();
  acl.srcPort() = portId;
  acl.dscp() = kNcnfDscp();
  acl.name() = getDscpReclassificationAclName();

  // To overcome DNX crash, add etherType to ACL
  utility::addEtherTypeToAcl(hwAsic, &acl, cfg::EtherType::IPv6);
  utility::addAclEntry(config, acl, utility::kDefaultAclTable());
  utility::addSetDscpActionToCfg(
      config, getDscpReclassificationAclName(), kIcpDscp());
}

void addDscpMarkingAcls(
    const HwAsic* hwAsic,
    cfg::SwitchConfig* config,
    bool isSai) {
  addDscpMarkingAclsHelper(
      hwAsic, config, IP_PROTO::IP_PROTO_UDP, kUdpPorts(), isSai);
  addDscpMarkingAclsHelper(
      hwAsic, config, IP_PROTO::IP_PROTO_TCP, kTcpPorts(), isSai);
}

void addDscpCounterAcl(
    const HwAsic* hwAsic,
    cfg::SwitchConfig* config,
    cfg::AclActionType actionType) {
  // Create ACL to count the number of packets with DSCP == ICP
  auto acl = utility::addDscpAclToCfg(
      hwAsic, config, kDscpCounterAclName(), utility::kIcpDscp());
  acl->actionType() = actionType;
  std::vector<cfg::CounterType> counterTypes =
      utility::getAclCounterTypes({hwAsic});
  utility::addTrafficCounter(config, kCounterName(), counterTypes);
  cfg::MatchAction matchAction = cfg::MatchAction();
  matchAction.counter() = kCounterName();
  utility::addMatcher(config, kDscpCounterAclName(), matchAction);
}

void delDscpMarkingAclMatchers(
    cfg::SwitchConfig* config,
    IP_PROTO proto,
    const std::vector<uint32_t>& ports) {
  for (auto port : ports) {
    auto l4SrcPortAclName = getDscpAclName(proto, "src", port);
    utility::delMatcher(config, l4SrcPortAclName);

    auto l4DstPortAclName = getDscpAclName(proto, "dst", port);
    utility::delMatcher(config, l4DstPortAclName);
  }
}

void delDscpMatchers(cfg::SwitchConfig* config) {
  utility::delAclStat(config, kDscpCounterAclName(), kCounterName());
  delDscpMarkingAclMatchers(config, IP_PROTO::IP_PROTO_UDP, kUdpPorts());
  delDscpMarkingAclMatchers(config, IP_PROTO::IP_PROTO_TCP, kTcpPorts());
}

void addDscpMarkingAclsTableHelper(
    cfg::SwitchConfig* config,
    IP_PROTO proto,
    const std::vector<uint32_t>& ports,
    const std::string& aclTableName,
    bool isSai) {
  HwAsicTable asicTable(
      config->switchSettings()->switchIdToSwitchInfo().value(),
      std::nullopt,
      *config->dsfNodes());
  auto asicType = checkSameAndGetAsicType(*config);
  for (auto port : ports) {
    cfg::AclEntry dscpSrcMarkingAcl;
    dscpSrcMarkingAcl.name() = getDscpAclName(proto, "src", port);
    dscpSrcMarkingAcl.actionType() = cfg::AclActionType::PERMIT;
    dscpSrcMarkingAcl.proto() = static_cast<int>(proto);
    dscpSrcMarkingAcl.l4SrcPort() = port;
    if (asicType == cfg::AsicType::ASIC_TYPE_CHENAB) {
      dscpSrcMarkingAcl.etherType() = cfg::EtherType::IPv6;
    }
    addAclEntry(config, dscpSrcMarkingAcl, aclTableName);

    utility::addSetDscpAndEgressQueueActionToCfg(
        checkSameAndGetAsic(asicTable.getL3Asics()),
        config,
        *dscpSrcMarkingAcl.name(),
        kIcpDscp(),
        utility::getOlympicQueueId(utility::OlympicQueueType::ICP),
        isSai);

    cfg::AclEntry dscpDstMarkingAcl;
    dscpDstMarkingAcl.name() = getDscpAclName(proto, "dst", port);
    dscpDstMarkingAcl.actionType() = cfg::AclActionType::PERMIT;
    dscpDstMarkingAcl.proto() = static_cast<int>(proto);
    dscpDstMarkingAcl.l4DstPort() = port;
    if (asicType == cfg::AsicType::ASIC_TYPE_CHENAB) {
      dscpDstMarkingAcl.etherType() = cfg::EtherType::IPv6;
    }
    utility::addAclEntry(config, dscpDstMarkingAcl, aclTableName);
    utility::addSetDscpAndEgressQueueActionToCfg(
        checkSameAndGetAsic(asicTable.getL3Asics()),
        config,
        *dscpDstMarkingAcl.name(),
        kIcpDscp(),
        utility::getOlympicQueueId(utility::OlympicQueueType::ICP),
        isSai);
  }
}

void addDscpMarkingAclTable(
    cfg::SwitchConfig* config,
    const std::string& aclTableName,
    bool isSai) {
  addDscpMarkingAclsTableHelper(
      config, IP_PROTO::IP_PROTO_UDP, kUdpPorts(), aclTableName, isSai);
  addDscpMarkingAclsTableHelper(
      config, IP_PROTO::IP_PROTO_TCP, kTcpPorts(), aclTableName, isSai);
}

void addDscpAclEntryWithCounter(
    cfg::SwitchConfig* config,
    const std::string& aclTableName,
    bool isSai) {
  std::vector<cfg::CounterType> counterTypes{cfg::CounterType::PACKETS};
  utility::addTrafficCounter(config, kCounterName(), counterTypes);
  cfg::AclEntry dscpAcl;
  dscpAcl.name() = kDscpCounterAclName();
  dscpAcl.actionType() = cfg::AclActionType::PERMIT;
  dscpAcl.dscp() = utility::kIcpDscp();
  utility::addAclEntry(config, dscpAcl, aclTableName);

  utility::addAclStat(
      config, kDscpCounterAclName(), kCounterName(), counterTypes);
  addDscpMarkingAclTable(config, aclTableName, isSai);
}

// Utility to add ICP Marking ACL table to a multi acl table group
void addDscpAclTable(
    cfg::SwitchConfig* config,
    const HwAsic* hwAsic,
    int16_t priority,
    bool addAllQualifiers,
    bool isSai) {
  std::vector<cfg::AclTableQualifier> qualifiers = {
      cfg::AclTableQualifier::L4_SRC_PORT,
      cfg::AclTableQualifier::L4_DST_PORT,
      cfg::AclTableQualifier::IP_PROTOCOL_NUMBER,
      cfg::AclTableQualifier::IPV6_NEXT_HEADER,
      cfg::AclTableQualifier::ICMPV4_TYPE,
      cfg::AclTableQualifier::ICMPV4_CODE,
      cfg::AclTableQualifier::ICMPV6_TYPE,
      cfg::AclTableQualifier::ICMPV6_CODE,
      cfg::AclTableQualifier::DSCP};
  if (addAllQualifiers) {
    // Add the extra qualifiers needed for Cisco Key profile
    qualifiers.push_back(cfg::AclTableQualifier::TTL);
    qualifiers.push_back(cfg::AclTableQualifier::DSCP);
  }
  if (hwAsic->isSupported(HwAsic::Feature::ACL_ENTRY_ETHER_TYPE)) {
    qualifiers.push_back(cfg::AclTableQualifier::ETHER_TYPE);
  }
  utility::addAclTable(
      config,
      getDscpAclTableName(),
      priority,
      {cfg::AclTableActionType::PACKET_ACTION,
       cfg::AclTableActionType::COUNTER,
       cfg::AclTableActionType::SET_TC,
       cfg::AclTableActionType::SET_DSCP},
      qualifiers);
  addDscpAclEntryWithCounter(config, getDscpAclTableName(), isSai);
}
} // namespace facebook::fboss::utility
