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

#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/bcm/BcmControlPlaneQueueManager.h"
#include "fboss/agent/hw/bcm/BcmCosManager.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmQosPolicyTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/PortQueue.h"

#include <thrift/lib/cpp/util/EnumUtils.h>

extern "C" {
#include <bcm/qos.h>
#include <bcm/types.h>
}

using facebook::fboss::cfg::PacketRxReason;
using facebook::stats::MonotonicCounter;
using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::system_clock;
namespace {

constexpr int kPriorityMaxValue = 127;

int getPriorityFromIndex(int index) {
  // larger value means higher priority
  return kPriorityMaxValue - index;
}

const std::set<PacketRxReason> kSupportedRxReasons = {
    PacketRxReason::UNMATCHED,
    PacketRxReason::ARP,
    PacketRxReason::DHCP,
    PacketRxReason::BPDU,
    PacketRxReason::L3_SLOW_PATH,
    PacketRxReason::L3_DEST_MISS,
    PacketRxReason::TTL_1,
    PacketRxReason::CPU_IS_NHOP,
    PacketRxReason::L3_MTU_ERROR};
} // namespace

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
    case cfg::PacketRxReason::L3_MTU_ERROR:
      return RxUtils::genReasons(bcmRxReasonL3MtuFail);
    case cfg::PacketRxReason::NDP:
    case cfg::PacketRxReason::LLDP:
    case cfg::PacketRxReason::ARP_RESPONSE:
    case cfg::PacketRxReason::BGP:
    case cfg::PacketRxReason::BGPV6:
    case cfg::PacketRxReason::LACP:
    case cfg::PacketRxReason::MPLS_TTL_1:
    case cfg::PacketRxReason::MPLS_UNKNOWN_LABEL:
    case cfg::PacketRxReason::DHCPV6:
    case cfg::PacketRxReason::SAMPLEPACKET:
    case cfg::PacketRxReason::TTL_0:
      break;
  }
  throw FbossError(
      "Unsupported config reason ", apache::thrift::util::enumName(reason));
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
  } else if (BCM_RX_REASON_GET(reasons, bcmRxReasonL3MtuFail)) {
    return cfg::PacketRxReason::L3_MTU_ERROR;
  } else {
    throw FbossError(
        "Unsupported bcm reasons ", RxUtils::describeReasons(reasons));
  }
}

BcmControlPlane::BcmControlPlane(BcmSwitch* hw)
    : hw_(hw),
      gport_(BCM_GPORT_LOCAL_CPU),
      queueManager_(new BcmControlPlaneQueueManager(hw_)) {
  int rv = bcm_rx_cosq_mapping_size_get(hw_->getUnit(), &maxRxReasonMappings_);
  bcmCheckError(rv, "failed to get max CPU cos queue mappings");
}

void BcmControlPlane::setupQueue(const PortQueue& queue) {
  queueManager_->program(queue);
}

ControlPlane::RxReasonToQueue BcmControlPlane::getRxReasonToQueue() const {
  ControlPlane::RxReasonToQueue reasonToQueue;
  for (int index = 0; index < maxRxReasonMappings_; ++index) {
    if (const auto entry = getReasonToQueueEntry(index)) {
      reasonToQueue.push_back(*entry);
    } else {
      break;
    }
  }
  return reasonToQueue;
}

int BcmControlPlane::getReservedBytes(const bcm_cos_queue_t queueId) {
  // cpu queues are mcast
  return queueManager_->getReservedBytes(
      cfg::StreamType::MULTICAST, gport_, queueId);
}

void BcmControlPlane::setReasonToQueueEntry(
    int index,
    cfg::PacketRxReasonToQueue entry) {
  auto maxCPUQueue = queueManager_->getNumQueues(cfg::StreamType::MULTICAST);
  auto queueID = *entry.queueId();
  if (queueID < 0 || queueID > maxCPUQueue) {
    throw FbossError("Invalud cosq number ", queueID, "; max is ", maxCPUQueue);
  }

  auto warmBootCache = hw_->getWarmBootCache();
  const auto cacheEntryItr = warmBootCache->findReasonToQueue(index);
  int rv;
  if (cacheEntryItr == warmBootCache->index2ReasonToQueue_end() ||
      cacheEntryItr->second != entry) {
    const auto bcmReason = configRxReasonToBcmReasons(*entry.rxReason());

    if (!hw_->useHSDK()) {
      rv = bcm_rx_cosq_mapping_set(
          hw_->getUnit(),
          index,
          bcmReason,
          bcmReason,
          0,
          0, // internal priority match & mask
          0,
          0, // packet type match & mask
          *entry.queueId());
    } else {
      rv = rxCosqMappingExtendedSet(
          hw_->getUnit(),
          index,
          bcmReason,
          bcmReason,
          0,
          0, // internal priority match & mask
          0,
          0, // packet type match & mask
          *entry.queueId());
    }
    bcmCheckError(
        rv,
        "failed to set CPU cosq mapping for reasons ",
        RxUtils::describeReasons(bcmReason));
  }
  if (cacheEntryItr != warmBootCache->index2ReasonToQueue_end()) {
    warmBootCache->programmed(cacheEntryItr);
  }
}

void BcmControlPlane::deleteReasonToQueueEntry(int index) {
  auto warmBootCache = hw_->getWarmBootCache();
  const auto cacheEntryItr = warmBootCache->findReasonToQueue(index);
  if (cacheEntryItr != warmBootCache->index2ReasonToQueue_end()) {
    warmBootCache->programmed(cacheEntryItr);
  }
  int rv;
  if (!hw_->useHSDK()) {
    rv = bcm_rx_cosq_mapping_delete(hw_->getUnit(), index);
  } else {
    bcm_rx_cosq_mapping_t cosqMap;
    bcm_rx_cosq_mapping_t_init(&cosqMap);

    cosqMap.index = index;
    cosqMap.priority = getPriorityFromIndex(index);
    rv = rxCosqMappingExtendedDelete(hw_->getUnit(), &cosqMap);
  }
  if (rv != BCM_E_NOT_FOUND) {
    bcmCheckError(rv, "failed to delete CPU cosq mapping for index ", index);
  } else {
    XLOG(WARNING) << "delete non-existent CPU cosq mapping at index " << index;
  }
}

std::optional<cfg::PacketRxReasonToQueue>
BcmControlPlane::getReasonToQueueEntry(int index) const {
  uint8_t prio, prioMask;
  uint32_t packetType, packetTypeMask;
  bcm_rx_reasons_t bcmReasons, reasonsMask;
  bcm_cos_queue_t queue;
  int rv;
  if (!hw_->useHSDK()) {
    rv = bcm_rx_cosq_mapping_get(
        hw_->getUnit(),
        index,
        &bcmReasons,
        &reasonsMask,
        &prio,
        &prioMask,
        &packetType,
        &packetTypeMask,
        &queue);
    if (BCM_FAILURE(rv)) {
      return std::nullopt;
    }
  } else {
    bcm_rx_cosq_mapping_t cosqMap;
    bcm_rx_cosq_mapping_t_init(&cosqMap);
    cosqMap.index = index;

    rv = rxCosqMappingExtendedGet(hw_->getUnit(), &cosqMap);
    if (BCM_FAILURE(rv)) {
      return std::nullopt;
    }
    bcmReasons = cosqMap.reasons;
    queue = cosqMap.cosq;
  }

  return ControlPlane::makeRxReasonToQueueEntry(
      bcmReasonsToConfigReason(bcmReasons), queue);
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

void BcmControlPlane::updateQueueCounters(HwPortStats* portStats) {
  auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
  queueManager_->updateQueueStats(now, portStats);
}

int BcmControlPlane::getCPUQueues() {
  return queueManager_->getNumQueues(cfg::StreamType::MULTICAST);
}

// wrapper for bcm_rx_cosq_mapping_extended_get
int BcmControlPlane::rxCosqMappingExtendedGet(
    int unit,
    bcm_rx_cosq_mapping_t* rx_cosq_mapping,
    bool byIndex) {
#if (!defined(IS_OPENNSA)) && (!defined(BCM_SDK_VERSION_GTE_6_5_20))
  return bcm_rx_cosq_mapping_get(
      unit,
      rx_cosq_mapping->index,
      &rx_cosq_mapping->reasons,
      &rx_cosq_mapping->reasons_mask,
      &rx_cosq_mapping->int_prio,
      &rx_cosq_mapping->int_prio_mask,
      &rx_cosq_mapping->packet_type,
      &rx_cosq_mapping->packet_type_mask,
      &rx_cosq_mapping->cosq);
#else
  bcm_rx_reasons_t reasons;
  auto targetIndex = rx_cosq_mapping->index;
  bcm_rx_reasons_t targetReasons = rx_cosq_mapping->reasons;
  bcm_rx_cosq_mapping_t_init(rx_cosq_mapping);
  int rv;
#if (defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_22))
  if (BcmAPI::isPriorityKeyUsedInRxCosqMapping()) {
    if (byIndex) {
      rx_cosq_mapping->priority = getPriorityFromIndex(targetIndex);
      rv = bcm_rx_cosq_mapping_extended_get(unit, rx_cosq_mapping);
      if (BCM_SUCCESS(rv)) {
        return rv;
      }
    } else {
      int maxSize;
      rv = bcm_rx_cosq_mapping_size_get(unit, &maxSize);
      bcmCheckError(rv, "failed to get max CPU cos queue mappings");
      for (int index = 0; index < maxSize; ++index) {
        bcm_rx_cosq_mapping_t_init(rx_cosq_mapping);
        rx_cosq_mapping->priority = getPriorityFromIndex(index);
        rv = bcm_rx_cosq_mapping_extended_get(unit, rx_cosq_mapping);
        if (BCM_SUCCESS(rv) &&
            BCM_RX_REASON_EQ(rx_cosq_mapping->reasons, targetReasons)) {
          return rv;
        }
      }
    }
    return BCM_E_NOT_FOUND;
  }
#endif

  for (std::set<cfg::PacketRxReason>::iterator it = kSupportedRxReasons.begin();
       it != kSupportedRxReasons.end();
       ++it) {
    reasons = configRxReasonToBcmReasons(*it);
    rx_cosq_mapping->reasons = reasons;
    rx_cosq_mapping->reasons_mask = reasons;
    rv = bcm_rx_cosq_mapping_extended_get(unit, rx_cosq_mapping);

    if (BCM_SUCCESS(rv) && (rx_cosq_mapping->index == targetIndex)) {
      return rv;
    }
  }
  return BCM_E_NOT_FOUND;
#endif
}

// wrapper for bcm_rx_cosq_mapping_extended_set
int BcmControlPlane::rxCosqMappingExtendedSet(
    int unit,
    int index,
    bcm_rx_reasons_t reasons,
    bcm_rx_reasons_t reasons_mask,
    uint8 int_prio,
    uint8 int_prio_mask,
    uint32 packet_type,
    uint32 packet_type_mask,
    bcm_cos_queue_t cosq) {
#if (!defined(IS_OPENNSA)) && (!defined(BCM_SDK_VERSION_GTE_6_5_20))
  return bcm_rx_cosq_mapping_set(
      unit,
      index,
      reasons,
      reasons_mask,
      int_prio,
      int_prio_mask, // internal priority match & mask
      packet_type,
      packet_type_mask, // packet type match & mask
      cosq);
#else
  bcm_rx_cosq_mapping_t cosqMap;
  bcm_rx_cosq_mapping_t_init(&cosqMap);

#if (defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_22))
  if (BcmAPI::isPriorityKeyUsedInRxCosqMapping()) {
    // use bcm_rx_cosq_mapping_extended_set
    cosqMap.reasons = reasons;
    auto rv = rxCosqMappingExtendedGet(unit, &cosqMap, false);
    if (rv != BCM_E_NOT_FOUND) {
      rxCosqMappingExtendedDelete(unit, &cosqMap);
    }
    bcm_rx_cosq_mapping_t_init(&cosqMap);
    cosqMap.priority = getPriorityFromIndex(index);
    cosqMap.reasons = reasons;
    cosqMap.reasons_mask = reasons_mask;
    cosqMap.int_prio = int_prio;
    cosqMap.int_prio_mask = int_prio_mask;
    cosqMap.packet_type = packet_type;
    cosqMap.packet_type_mask = packet_type_mask;
    cosqMap.cosq = cosq;
    return bcm_rx_cosq_mapping_extended_set(unit, 0, &cosqMap);
  } // else use bcm_rx_cosq_mapping_extended_add
#endif

  // remove existing entry with the table index to be used
  cosqMap.index = index;
  auto rv = rxCosqMappingExtendedGet(unit, &cosqMap);
  if (rv != BCM_E_NOT_FOUND) {
    rxCosqMappingExtendedDelete(unit, &cosqMap);
  }

  // update existing reason if it exists
  int options = 0;
  bcm_rx_cosq_mapping_t_init(&cosqMap);
  cosqMap.reasons = reasons;
  cosqMap.reasons_mask = reasons_mask;
  rv = bcm_rx_cosq_mapping_extended_get(unit, &cosqMap);
  if (BCM_SUCCESS(rv)) {
    options = BCM_RX_COSQ_MAPPING_OPTIONS_REPLACE;
  }

  bcm_rx_cosq_mapping_t_init(&cosqMap);
  cosqMap.index = index;
  cosqMap.reasons = reasons;
  cosqMap.reasons_mask = reasons_mask;
  cosqMap.int_prio = int_prio;
  cosqMap.int_prio_mask = int_prio_mask;
  cosqMap.packet_type = packet_type;
  cosqMap.packet_type_mask = packet_type_mask;
  cosqMap.cosq = cosq;
  return bcm_rx_cosq_mapping_extended_add(unit, options, &cosqMap);
#endif
}

// wrapper for bcm_rx_cosq_mapping_extended_delete
int BcmControlPlane::rxCosqMappingExtendedDelete(
    int unit,
    bcm_rx_cosq_mapping_t* rx_cosq_mapping) {
#if (!defined(IS_OPENNSA)) && (!defined(BCM_SDK_VERSION_GTE_6_5_20))
  return bcm_rx_cosq_mapping_delete(unit, rx_cosq_mapping->index);
#else
  return bcm_rx_cosq_mapping_extended_delete(unit, rx_cosq_mapping);
#endif
}

void BcmControlPlane::updateQueueWatermarks(HwPortStats* portStats) {
  if (!portStats ||
      !hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }
  for (const auto cosQ :
       queueManager_->getNamedQueues(cfg::StreamType::MULTICAST)) {
    uint64_t peakCells = hw_->getCosMgr()->cpuStatGet(cosQ, bcmBstStatIdMcast);
    portStats->queueWatermarkBytes_()[cosQ] =
        peakCells * hw_->getMMUCellBytes();
  }
}

} // namespace facebook::fboss
