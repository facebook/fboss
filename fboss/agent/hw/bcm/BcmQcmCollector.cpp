/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmQcmCollector.h"
#include <folly/logging/xlog.h>
#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorUtils.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmQcmManager.h"
#include "fboss/agent/hw/bcm/BcmSdkVer.h"
#include "fboss/agent/state/QcmConfig.h"
#include "fboss/agent/state/SwitchState.h"

extern "C" {
#include <bcm/collector.h>
#include <bcm/l2.h>
#include <bcm/l3.h>
#include <bcm/types.h>
#if (defined(IS_OPENNSA))
#define sal_memset memset
#define BCM_WARM_BOOT_SUPPORT
#include <soc/opensoc.h>
#else
#include <soc/drv.h>
#endif
}

namespace {
constexpr int kQcmCollectorId =
    4; // arbitrarily picked id to identify collector
constexpr int kQcmCollectorExportProfileId = 8; // arbitrarily picked id
// Arbitrary macs for collector
// Used to create station table entry
const folly::MacAddress kCollectorSrcMac("00:00:00:44:44:44");
const folly::MacAddress kCollectorDstMac("00:00:00:33:33:33");
// Configure the collector pkts to use arbit large ttl value
constexpr int kCollectorTtl = 127;
} // namespace

namespace facebook::fboss {

int BcmQcmCollector::getQcmCollectorId() {
  return kQcmCollectorId;
}

int BcmQcmCollector::getQcmCollectorExportProfileId() {
  return kQcmCollectorExportProfileId;
}

folly::MacAddress BcmQcmCollector::getCollectorDstMac() {
  return kCollectorDstMac;
}

void BcmQcmCollector::populateCollectorIPInfo(
    bcm_collector_info_t& collectorInfo) {
  auto collectorSrcIPCidr = qcmCfg_->getCollectorSrcIp();
  auto collectorDstIPCidr = qcmCfg_->getCollectorDstIp();
  uint8_t dscpVal = 0;

  if (auto dscp = qcmCfg_->getCollectorDscp()) {
    dscpVal = *dscp;
  }

  if (collectorSrcIPCidr.first.isV6()) {
    ipToBcmIp6(collectorSrcIPCidr.first, &collectorInfo.ipv6.src_ip);
    ipToBcmIp6(collectorDstIPCidr.first, &collectorInfo.ipv6.dst_ip);
    collectorInfo.ipv6.hop_limit = kCollectorTtl;
    collectorInfo.ipv6.traffic_class = dscpVal;
    collectorInfo.udp.flags = BCM_COLLECTOR_UDP_FLAGS_CHECKSUM_ENABLE;
    collectorInfo.transport_type = bcmCollectorTransportTypeIpv6Udp;
  } else {
    collectorInfo.ipv4.src_ip = collectorSrcIPCidr.first.asV4().toLongHBO();
    collectorInfo.ipv4.dst_ip = collectorDstIPCidr.first.asV4().toLongHBO();
    collectorInfo.ipv4.dscp = dscpVal;
    collectorInfo.ipv4.ttl = kCollectorTtl;
    collectorInfo.transport_type = bcmCollectorTransportTypeIpv4Udp;
  }
}

void BcmQcmCollector::collectorCreate() {
  bcm_collector_info_t collectorInfo;
  bcm_collector_info_t_init(&collectorInfo);

  macToBcm(kCollectorDstMac, &collectorInfo.eth.dst_mac);
  macToBcm(kCollectorSrcMac, &collectorInfo.eth.src_mac);
  collectorInfo.eth.vlan_tag_structure = BCM_COLLECTOR_ETH_HDR_SINGLE_TAGGED;
  collectorInfo.eth.outer_vlan_tag = collector_vlan_;
  collectorInfo.eth.outer_tpid = 0x8100; // 802.1Q
  collectorInfo.udp.src_port = qcmCfg_->getCollectorSrcPort();
  collectorInfo.udp.dst_port = qcmCfg_->getCollectorDstPort();
  collectorInfo.collector_type = bcmCollectorRemote;

  populateCollectorIPInfo(collectorInfo);
  int collector_id = kQcmCollectorId;
  auto rv = bcm_collector_create(
      hw_->getUnit(), BCM_COLLECTOR_WITH_ID, &(collector_id), &collectorInfo);
  bcmCheckError(rv, "bcm_collector_create failed");
}

void BcmQcmCollector::exportProfileCreate() {
  bcm_collector_export_profile_t profile_info;

  bcm_collector_export_profile_t_init(&profile_info);
  profile_info.wire_format = bcmCollectorWireFormatIpfix;
  profile_info.max_packet_length = 1500;

  int export_profile_id = kQcmCollectorExportProfileId;
  auto rv = bcm_collector_export_profile_create(
      hw_->getUnit(),
      BCM_COLLECTOR_EXPORT_PROFILE_WITH_ID,
      &export_profile_id,
      &profile_info);
  bcmCheckError(rv, "bcm_collector_export_profile_create failed");
}

bool BcmQcmCollector::getCollectorVlan(
    const std::shared_ptr<SwitchState>& swState) {
  const auto vlans = swState->getVlans();
  for (const auto& [id, vlan] : std::as_const(*vlans)) {
    // pick up any arbitrary vlan which is programmed
    // Collector pkts need to hit my station table entry
    // to route which can happen based on {dst_mac, vlan}
    // vlan is picked to be any configured vlan
    // for convinence.
    collector_vlan_ = vlan->getID();
    return true;
  }
  return false;
}

void BcmQcmCollector::setupL3Collector() {
  bcm_l3_intf_t l3_intf;
  bcm_l3_intf_t_init(&l3_intf);
  bcm_l2_addr_t l2addr;

  // setup l3 interface for the collector vlan + collector dst ip
  l3_intf.l3a_vid = collector_vlan_;
  l3_intf.l3a_vrf = 0;
  macToBcm(kCollectorDstMac, &l3_intf.l3a_mac_addr);

  int rv = bcm_l3_intf_create(hw_->getUnit(), &l3_intf);
  bcmCheckError(rv, "bcm_l3_intf_create failed");

  // program my station table for given vlan, mac
  bcm_l2_addr_t_init(&l2addr, l3_intf.l3a_mac_addr, collector_vlan_);
  l2addr.flags |= (BCM_L2_L3LOOKUP | BCM_L2_STATIC);
  rv = bcm_l2_addr_add(hw_->getUnit(), &l2addr);
  bcmCheckError(rv, "bcm_l2_addr_addr failed");
}

void BcmQcmCollector::init(const std::shared_ptr<SwitchState>& swState) {
  qcmCfg_ = swState->getQcmCfg();
  if (!getCollectorVlan(swState)) {
    XLOG(ERR) << "No vlan configured. Collector cannot be setup";
    return;
  }
  setupL3Collector();
  collectorCreate();
  exportProfileCreate();
  flowTrackerTemplateCreate();
  collectorTemplateAttach();
  XLOG(DBG2) << "Collector Init done";
}

void BcmQcmCollector::destroyCollector() {
  auto rv = bcm_collector_destroy(hw_->getUnit(), kQcmCollectorId);
  bcmCheckError(rv, "bcm_collector_destroy failed");
}

void BcmQcmCollector::destroyExportProfile() {
  auto rv = bcm_collector_export_profile_destroy(
      hw_->getUnit(), kQcmCollectorExportProfileId);
  bcmCheckError(rv, "bcm_collector_export_profile_destroy failed");
}

void BcmQcmCollector::stop() {
  collectorTemplateDetach();
  destroyCollector();
  destroyExportProfile();
  destroyFlowTrackerTemplate();

  XLOG(DBG2) << "Collector stop done";
}
} // namespace facebook::fboss
