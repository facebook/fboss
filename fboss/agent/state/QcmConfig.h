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

#include <folly/IPAddress.h>
#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/NodeBase.h"

namespace facebook::fboss {

class SwitchState;

typedef boost::container::flat_map<int, int> WeightMap;
typedef std::map<int, std::set<int>> Port2QosQueueIdMap;

struct QcmCfgFields {
  template <typename Fn>
  void forEachChild(Fn /*fn*/) {}

  folly::dynamic toFollyDynamic() const;
  static QcmCfgFields fromFollyDynamic(const folly::dynamic& json);

  uint32_t agingIntervalInMsecs{
      cfg::switch_config_constants::DEFAULT_QCM_AGING_INTERVAL_MSECS()};
  uint32_t numFlowSamplesPerView{
      cfg::switch_config_constants::DEFAULT_QCM_FLOWS_PER_VIEW()};
  uint32_t flowLimit{cfg::switch_config_constants::DEFAULT_QCM_FLOW_LIMIT()};
  uint32_t numFlowsClear{
      cfg::switch_config_constants::DEFAULT_QCM_NUM_FLOWS_TO_CLEAR()};
  uint32_t scanIntervalInUsecs{
      cfg::switch_config_constants::DEFAULT_QCM_SCAN_INTERVAL_USECS()};
  uint32_t exportThreshold{
      cfg::switch_config_constants::DEFAULT_QCM_EXPORT_THRESHOLD()};
  bool monitorQcmCfgPortsOnly{false};

  WeightMap flowWeights;
  folly::CIDRNetwork collectorDstIp{std::make_pair(folly::IPAddress(), 0)};
  folly::CIDRNetwork collectorSrcIp{std::make_pair(folly::IPAddress(), 0)};
  uint32_t collectorSrcPort;
  uint32_t collectorDstPort{
      cfg::switch_config_constants::DEFAULT_QCM_COLLECTOR_DST_PORT()};
  std::optional<uint32_t> collectorDscp{std::nullopt};
  std::optional<uint32_t> ppsToQcm{std::nullopt};
  std::vector<int32_t> monitorQcmPortList;
  Port2QosQueueIdMap port2QosQueueIds;
};

class QcmCfg : public NodeBaseT<QcmCfg, QcmCfgFields> {
 public:
  static std::shared_ptr<QcmCfg> fromFollyDynamic(const folly::dynamic& json) {
    const auto& fields = QcmCfgFields::fromFollyDynamic(json);
    return std::make_shared<QcmCfg>(fields);
  }

  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamic();
  }

  bool operator==(const QcmCfg& qcm) const {
    auto origQcmMonitoredPorts{getFields()->monitorQcmPortList};
    auto newQcmMonitoredPorts{qcm.getMonitorQcmPortList()};
    sort(origQcmMonitoredPorts.begin(), origQcmMonitoredPorts.end());
    sort(newQcmMonitoredPorts.begin(), newQcmMonitoredPorts.end());
    bool qcmMonitoredPortsNotChanged =
        (origQcmMonitoredPorts == newQcmMonitoredPorts);

    return getFields()->agingIntervalInMsecs == qcm.getAgingInterval() &&
        getFields()->numFlowSamplesPerView == qcm.getNumFlowSamplesPerView() &&
        getFields()->flowLimit == qcm.getFlowLimit() &&
        getFields()->numFlowsClear == qcm.getNumFlowsClear() &&
        getFields()->scanIntervalInUsecs == qcm.getScanIntervalInUsecs() &&
        getFields()->exportThreshold == qcm.getExportThreshold() &&
        getFields()->collectorDstIp == qcm.getCollectorDstIp() &&
        getFields()->collectorSrcIp == qcm.getCollectorSrcIp() &&
        getFields()->collectorDstPort == qcm.getCollectorDstPort() &&
        getFields()->collectorDscp == qcm.getCollectorDscp() &&
        getFields()->ppsToQcm == qcm.getPpsToQcm() &&
        getFields()->monitorQcmCfgPortsOnly ==
        qcm.getMonitorQcmCfgPortsOnly() &&
        getFields()->flowWeights == qcm.getFlowWeightMap() &&
        getFields()->port2QosQueueIds == qcm.getPort2QosQueueIdMap() &&
        qcmMonitoredPortsNotChanged;
  }

  bool operator!=(const QcmCfg& qcm) const {
    return !operator==(qcm);
  }
  uint32_t getAgingInterval() const {
    return getFields()->agingIntervalInMsecs;
  }

  void setAgingInterval(uint32_t agingInterval) {
    writableFields()->agingIntervalInMsecs = agingInterval;
  }

  void setNumFlowSamplesPerView(uint32_t numFlowSamplesPerView) {
    writableFields()->numFlowSamplesPerView = numFlowSamplesPerView;
  }

  uint32_t getNumFlowSamplesPerView() const {
    return getFields()->numFlowSamplesPerView;
  }

  void setFlowLimit(uint32_t flowLimit) {
    writableFields()->flowLimit = flowLimit;
  }

  uint32_t getFlowLimit() const {
    return getFields()->flowLimit;
  }

  void setNumFlowsClear(uint32_t numFlowsClear) {
    writableFields()->numFlowsClear = numFlowsClear;
  }

  uint32_t getNumFlowsClear() const {
    return getFields()->numFlowsClear;
  }

  void setScanIntervalInUsecs(uint32_t scanInterval) {
    writableFields()->scanIntervalInUsecs = scanInterval;
  }

  uint32_t getScanIntervalInUsecs() const {
    return getFields()->scanIntervalInUsecs;
  }

  void setExportThreshold(uint32_t exportThreshold) {
    writableFields()->exportThreshold = exportThreshold;
  }

  uint32_t getExportThreshold() const {
    return getFields()->exportThreshold;
  }

  WeightMap getFlowWeightMap() const {
    return getFields()->flowWeights;
  }

  void setFlowWeightMap(WeightMap map) {
    writableFields()->flowWeights = map;
  }

  Port2QosQueueIdMap getPort2QosQueueIdMap() const {
    return getFields()->port2QosQueueIds;
  }

  void setPort2QosQueueIdMap(Port2QosQueueIdMap& map) {
    writableFields()->port2QosQueueIds = map;
  }

  folly::CIDRNetwork getCollectorDstIp() const {
    return getFields()->collectorDstIp;
  }

  void setCollectorDstIp(const folly::CIDRNetwork& ip) {
    writableFields()->collectorDstIp = ip;
  }

  folly::CIDRNetwork getCollectorSrcIp() const {
    return getFields()->collectorSrcIp;
  }

  void setCollectorSrcIp(const folly::CIDRNetwork& ip) {
    writableFields()->collectorSrcIp = ip;
  }

  uint32_t getCollectorSrcPort() const {
    return getFields()->collectorSrcPort;
  }

  void setCollectorSrcPort(const uint32_t srcPort) {
    writableFields()->collectorSrcPort = srcPort;
  }

  void setCollectorDstPort(const uint32_t dstPort) {
    writableFields()->collectorDstPort = dstPort;
  }

  uint32_t getCollectorDstPort() const {
    return getFields()->collectorDstPort;
  }

  std::optional<uint32_t> getCollectorDscp() const {
    return getFields()->collectorDscp;
  }

  void setCollectorDscp(const uint32_t dscp) {
    writableFields()->collectorDscp = dscp;
  }

  std::optional<uint32_t> getPpsToQcm() const {
    return getFields()->ppsToQcm;
  }

  void setPpsToQcm(const uint32_t ppsToQcm) {
    writableFields()->ppsToQcm = ppsToQcm;
  }

  const std::vector<int32_t>& getMonitorQcmPortList() const {
    return getFields()->monitorQcmPortList;
  }
  void setMonitorQcmPortList(const std::vector<int32_t>& qcmPortList) {
    writableFields()->monitorQcmPortList = qcmPortList;
  }

  bool getMonitorQcmCfgPortsOnly() const {
    return getFields()->monitorQcmCfgPortsOnly;
  }
  void setMonitorQcmCfgPortsOnly(bool monitorQcmPortsOnly) {
    writableFields()->monitorQcmCfgPortsOnly = monitorQcmPortsOnly;
  }

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
