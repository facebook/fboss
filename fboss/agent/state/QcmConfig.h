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

#include <fboss/agent/AddressUtil.h>
#include <folly/IPAddress.h>
#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"

namespace facebook::fboss {

class SwitchState;

typedef std::map<int, int> WeightMap;
typedef std::map<int, std::set<int>> Port2QosQueueIdMap;

USE_THRIFT_COW(QcmCfg);

class QcmCfg : public ThriftStructNode<QcmCfg, state::QcmCfgFields> {
 public:
  using Base = ThriftStructNode<QcmCfg, state::QcmCfgFields>;

  uint32_t getAgingInterval() const {
    return get<switch_state_tags::agingIntervalInMsecs>()->cref();
  }

  void setAgingInterval(uint32_t agingInterval) {
    set<switch_state_tags::agingIntervalInMsecs>(agingInterval);
  }

  void setNumFlowSamplesPerView(uint32_t numFlowSamplesPerView) {
    set<switch_state_tags::numFlowSamplesPerView>(numFlowSamplesPerView);
  }

  uint32_t getNumFlowSamplesPerView() const {
    return get<switch_state_tags::numFlowSamplesPerView>()->cref();
  }

  void setFlowLimit(uint32_t flowLimit) {
    set<switch_state_tags::flowLimit>(flowLimit);
  }

  uint32_t getFlowLimit() const {
    return get<switch_state_tags::flowLimit>()->cref();
  }

  void setNumFlowsClear(uint32_t numFlowsClear) {
    set<switch_state_tags::numFlowsClear>(numFlowsClear);
  }

  uint32_t getNumFlowsClear() const {
    return get<switch_state_tags::numFlowsClear>()->cref();
  }

  void setScanIntervalInUsecs(uint32_t scanInterval) {
    set<switch_state_tags::scanIntervalInUsecs>(scanInterval);
  }

  uint32_t getScanIntervalInUsecs() const {
    return get<switch_state_tags::scanIntervalInUsecs>()->cref();
  }

  void setExportThreshold(uint32_t exportThreshold) {
    set<switch_state_tags::exportThreshold>(exportThreshold);
  }

  uint32_t getExportThreshold() const {
    return get<switch_state_tags::exportThreshold>()->cref();
  }

  auto getFlowWeightMap() const {
    return get<switch_state_tags::flowWeights>();
  }

  void setFlowWeightMap(WeightMap map) {
    set<switch_state_tags::flowWeights>(map);
  }

  auto getPort2QosQueueIdMap() const {
    return get<switch_state_tags::port2QosQueueIds>();
  }

  void setPort2QosQueueIdMap(Port2QosQueueIdMap& map) {
    set<switch_state_tags::port2QosQueueIds>(map);
  }

  folly::CIDRNetwork getCollectorDstIp() const {
    return ThriftyUtils::toCIDRNetwork(
        get<switch_state_tags::collectorDstIp>()->toThrift());
  }

  void setCollectorDstIp(const folly::CIDRNetwork& ip) {
    set<switch_state_tags::collectorDstIp>(ThriftyUtils::toIpPrefix(ip));
  }

  folly::CIDRNetwork getCollectorSrcIp() const {
    return ThriftyUtils::toCIDRNetwork(
        get<switch_state_tags::collectorSrcIp>()->toThrift());
  }

  void setCollectorSrcIp(const folly::CIDRNetwork& ip) {
    set<switch_state_tags::collectorSrcIp>(ThriftyUtils::toIpPrefix(ip));
  }

  uint16_t getCollectorSrcPort() const {
    return get<switch_state_tags::collectorSrcPort>()->cref();
  }

  void setCollectorSrcPort(const uint16_t srcPort) {
    set<switch_state_tags::collectorSrcPort>(srcPort);
  }

  void setCollectorDstPort(const uint16_t dstPort) {
    set<switch_state_tags::collectorDstPort>(dstPort);
  }

  uint16_t getCollectorDstPort() const {
    return get<switch_state_tags::collectorDstPort>()->cref();
  }

  std::optional<uint8_t> getCollectorDscp() const {
    if (auto dscp = get<switch_state_tags::collectorDscp>()) {
      return dscp->cref();
    }
    return std::nullopt;
  }

  void setCollectorDscp(const uint8_t dscp) {
    set<switch_state_tags::collectorDscp>(dscp);
  }

  std::optional<uint32_t> getPpsToQcm() const {
    if (auto ppsToQcm = get<switch_state_tags::ppsToQcm>()) {
      return ppsToQcm->cref();
    }
    return std::nullopt;
  }

  void setPpsToQcm(const uint32_t ppsToQcm) {
    set<switch_state_tags::ppsToQcm>(ppsToQcm);
  }

  auto getMonitorQcmPortList() const {
    return get<switch_state_tags::monitorQcmPortList>();
  }
  void setMonitorQcmPortList(const std::vector<int32_t>& qcmPortList) {
    set<switch_state_tags::monitorQcmPortList>(qcmPortList);
  }

  bool getMonitorQcmCfgPortsOnly() const {
    return get<switch_state_tags::monitorQcmCfgPortsOnly>()->cref();
  }
  void setMonitorQcmCfgPortsOnly(bool monitorQcmPortsOnly) {
    set<switch_state_tags::monitorQcmCfgPortsOnly>(monitorQcmPortsOnly);
  }

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
