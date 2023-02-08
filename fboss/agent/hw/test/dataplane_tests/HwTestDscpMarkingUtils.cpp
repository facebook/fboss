/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/dataplane_tests/HwTestDscpMarkingUtils.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/TrafficPolicyUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.h"

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
  return utility::kOlympicQueueToDscp().at(utility::kOlympicICPQueueId).front();
}

std::string
getDscpAclName(IP_PROTO proto, std::string direction, uint32_t port) {
  return folly::to<std::string>(
      "dscp-mark-for-proto-",
      static_cast<int>(proto),
      "-L4-",
      direction,
      "-port-",
      port);
}

void addDscpMarkingAclsHelper(
    cfg::SwitchConfig* config,
    IP_PROTO proto,
    const std::vector<uint32_t>& ports) {
  for (auto port : ports) {
    auto l4SrcPortAclName = getDscpAclName(proto, "src", port);
    utility::addL4SrcPortAclToCfg(config, l4SrcPortAclName, proto, port);
    utility::addSetDscpAndEgressQueueActionToCfg(
        config, l4SrcPortAclName, kIcpDscp(), utility::kOlympicICPQueueId);

    auto l4DstPortAclName = getDscpAclName(proto, "dst", port);
    utility::addL4DstPortAclToCfg(config, l4DstPortAclName, proto, port);
    utility::addSetDscpAndEgressQueueActionToCfg(
        config, l4DstPortAclName, kIcpDscp(), utility::kOlympicICPQueueId);
  }
}

void addDscpMarkingAcls(cfg::SwitchConfig* config) {
  addDscpMarkingAclsHelper(config, IP_PROTO::IP_PROTO_UDP, kUdpPorts());
  addDscpMarkingAclsHelper(config, IP_PROTO::IP_PROTO_TCP, kTcpPorts());
}

void addDscpCounterAcl(cfg::SwitchConfig* config) {
  // Create ACL to count the number of packets with DSCP == ICP
  utility::addDscpAclToCfg(config, kDscpCounterAclName(), utility::kIcpDscp());
  std::vector<cfg::CounterType> counterTypes{cfg::CounterType::PACKETS};
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
    const std::string& aclTableName) {
  for (auto port : ports) {
    auto l4SrcPortAclName = getDscpAclName(proto, "src", port);
    auto dscpSrcMarkingAcl = utility::addAcl(
        config, l4SrcPortAclName, cfg::AclActionType::PERMIT, aclTableName);
    dscpSrcMarkingAcl->proto() = static_cast<int>(proto);
    dscpSrcMarkingAcl->l4SrcPort() = port;
    utility::addSetDscpAndEgressQueueActionToCfg(
        config, l4SrcPortAclName, kIcpDscp(), utility::kOlympicICPQueueId);

    auto l4DstPortAclName = getDscpAclName(proto, "dst", port);
    auto dscpDstMarkingAcl = utility::addAcl(
        config, l4DstPortAclName, cfg::AclActionType::PERMIT, aclTableName);
    dscpDstMarkingAcl->proto() = static_cast<int>(proto);
    dscpDstMarkingAcl->l4DstPort() = port;
    utility::addSetDscpAndEgressQueueActionToCfg(
        config, l4DstPortAclName, kIcpDscp(), utility::kOlympicICPQueueId);
  }
}

void addDscpMarkingAclTable(
    cfg::SwitchConfig* config,
    const std::string& aclTableName) {
  addDscpMarkingAclsTableHelper(
      config, IP_PROTO::IP_PROTO_UDP, kUdpPorts(), aclTableName);
  addDscpMarkingAclsTableHelper(
      config, IP_PROTO::IP_PROTO_TCP, kTcpPorts(), aclTableName);
}

void addDscpAclEntryWithCounter(
    cfg::SwitchConfig* config,
    const std::string& aclTableName) {
  std::vector<cfg::CounterType> counterTypes{cfg::CounterType::PACKETS};
  utility::addTrafficCounter(config, kCounterName(), counterTypes);
  auto* dscpAcl = utility::addAcl(
      config, kDscpCounterAclName(), cfg::AclActionType::PERMIT, aclTableName);
  dscpAcl->dscp() = utility::kIcpDscp();

  utility::addAclStat(
      config, kDscpCounterAclName(), kCounterName(), counterTypes);
  addDscpMarkingAclTable(config, aclTableName);
}

// Utility to add ICP Marking ACL table to a multi acl table group
void addDscpAclTable(cfg::SwitchConfig* config, int16_t priority) {
  std::vector<cfg::AclTableQualifier> qualifiers = {
      cfg::AclTableQualifier::L4_SRC_PORT,
      cfg::AclTableQualifier::L4_DST_PORT,
      cfg::AclTableQualifier::IP_PROTOCOL,
      cfg::AclTableQualifier::ICMPV4_TYPE,
      cfg::AclTableQualifier::ICMPV4_CODE,
      cfg::AclTableQualifier::ICMPV6_TYPE,
      cfg::AclTableQualifier::ICMPV6_CODE,
      cfg::AclTableQualifier::DSCP};
#if defined(TAJO_SDK_VERSION_1_58_0) || defined(TAJO_SDK_VERSION_1_60_0)
  qualifiers.push_back(cfg::AclTableQualifier::TTL);
#endif
  utility::addAclTable(
      config,
      getDscpAclTableName(),
      priority,
      {cfg::AclTableActionType::PACKET_ACTION,
       cfg::AclTableActionType::COUNTER,
       cfg::AclTableActionType::SET_TC,
       cfg::AclTableActionType::SET_DSCP},
      qualifiers);

  addDscpAclEntryWithCounter(config, getDscpAclTableName());
}
} // namespace facebook::fboss::utility
