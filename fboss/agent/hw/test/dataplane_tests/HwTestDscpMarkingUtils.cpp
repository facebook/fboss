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

#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/TrafficPolicyUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.h"

#include <string>

namespace facebook::fboss::utility {

// 53: DNS, 88: Kerberos, 123: NTP, 319/320: PTP, 514: syslog, 636: LDAP

const std::vector<uint32_t>& kTcpPorts() {
  static const std::vector<uint32_t> tcpPorts = {53, 88, 123, 636};

  return tcpPorts;
}

const std::vector<uint32_t>& kUdpPorts() {
  static const std::vector<uint32_t> udpPorts = {53, 88, 123, 319, 320, 514};

  return udpPorts;
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

} // namespace facebook::fboss::utility
