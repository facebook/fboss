/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmControlPlane.h"

#include "fboss/agent/hw/bcm/BcmControlPlaneQueueManager.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmQosPolicyTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/state/PortQueue.h"

#include <thrift/lib/cpp/util/EnumUtils.h>

extern "C" {
#include <bcm/qos.h>
#include <bcm/types.h>
}

using facebook::stats::MonotonicCounter;
using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::system_clock;

constexpr auto kCPUName = "cpu";

namespace facebook::fboss {

bcm_rx_reasons_t configRxReasonToBcmReasons(cfg::PacketRxReason reason) {
  switch (reason) {
    case cfg::PacketRxReason::UNMATCHED:
      return RxUtils::genReasons();
    case cfg::PacketRxReason::ARP:
      return RxUtils::genReasons(bcmRxReasonArp);
    case cfg::PacketRxReason::DHCP:
      return RxUtils::genReasons(bcmRxReasonDhcp);
    case cfg::PacketRxReason::BPDU:
      return RxUtils::genReasons(bcmRxReasonBpdu);
    case cfg::PacketRxReason::L3_SLOW_PATH:
      return RxUtils::genReasons(bcmRxReasonL3Slowpath);
    case cfg::PacketRxReason::L3_DEST_MISS:
      return RxUtils::genReasons(bcmRxReasonL3DestMiss);
    case cfg::PacketRxReason::TTL_1:
      return RxUtils::genReasons(bcmRxReasonTtl1);
    case cfg::PacketRxReason::CPU_IS_NHOP:
      return RxUtils::genReasons(bcmRxReasonNhop);
    case cfg::PacketRxReason::NDP:
    case cfg::PacketRxReason::LLDP:
    case cfg::PacketRxReason::ARP_RESPONSE:
    case cfg::PacketRxReason::BGP:
    case cfg::PacketRxReason::BGPV6:
    case cfg::PacketRxReason::LACP:
      throw FbossError(
          "Unsupported config reason ", apache::thrift::util::enumName(reason));
  }
}

cfg::PacketRxReason bcmReasonsToConfigReason(bcm_rx_reasons_t reasons) {
  int count = 0;
  BCM_RX_REASON_COUNT(reasons, count);
  CHECK_LE(count, 1); // we only set reasons one at a time right now
  if (count == 0) {
    return cfg::PacketRxReason::UNMATCHED;
  } else if (BCM_RX_REASON_GET(reasons, bcmRxReasonArp)) {
    return cfg::PacketRxReason::ARP;
  } else if (BCM_RX_REASON_GET(reasons, bcmRxReasonDhcp)) {
    return cfg::PacketRxReason::DHCP;
  } else if (BCM_RX_REASON_GET(reasons, bcmRxReasonBpdu)) {
    return cfg::PacketRxReason::BPDU;
  } else if (BCM_RX_REASON_GET(reasons, bcmRxReasonL3Slowpath)) {
    return cfg::PacketRxReason::L3_SLOW_PATH;
  } else if (BCM_RX_REASON_GET(reasons, bcmRxReasonL3DestMiss)) {
    return cfg::PacketRxReason::L3_DEST_MISS;
  } else if (BCM_RX_REASON_GET(reasons, bcmRxReasonTtl1)) {
    return cfg::PacketRxReason::TTL_1;
  } else if (BCM_RX_REASON_GET(reasons, bcmRxReasonNhop)) {
    return cfg::PacketRxReason::CPU_IS_NHOP;
  } else {
    throw FbossError(
        "Unsupported bcm reasons ", RxUtils::describeReasons(reasons));
  }
}

BcmControlPlane::BcmControlPlane(BcmSwitch* hw)
    : hw_(hw),
      gport_(BCM_GPORT_LOCAL_CPU),
      queueManager_(new BcmControlPlaneQueueManager(hw_, kCPUName, gport_)) {}

void BcmControlPlane::setupQueue(const PortQueue& queue) {
  queueManager_->program(queue);
}

void BcmControlPlane::setupRxReasonToQueue(
    const ControlPlane::RxReasonToQueue& /*reasonToQueue*/) {
  // TODO(joseph5wu) Implement the logic of setting reason mapping to bcm
}

void BcmControlPlane::setupIngressQosPolicy(
    const std::optional<std::string>& qosPolicyName) {
  int ingressHandle = 0; // 0 means 'detach the current policy'
  int egressHandle = -1; // -1 means do not change the current policy
  if (qosPolicyName) {
    auto qosPolicy = hw_->getQosPolicyTable()->getQosPolicy(*qosPolicyName);
    ingressHandle = qosPolicy->getHandle(BcmQosMap::Type::IP_INGRESS);
  }
  auto rv =
      bcm_qos_port_map_set(hw_->getUnit(), 0, ingressHandle, egressHandle);
  bcmCheckError(rv, "failed to set the qos map on CPU port");
}

void BcmControlPlane::updateQueueCounters() {
  auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
  queueManager_->updateQueueStats(now);
}

} // namespace facebook::fboss
