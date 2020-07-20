/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/QcmConfig.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/state/NodeBase-defs.h"

namespace {
constexpr auto kNumFlowSamplesPerView = "numFlowSamplesPerView";
constexpr auto kFlowLimit = "flowLimit";
constexpr auto kNumFlowsClear = "numFlowsClear";
constexpr auto kScanIntervalInUsecs = "scanIntervalInUsecs";
constexpr auto kExportThreshold = "exportThreshold";
constexpr auto kFlowWeights = "flowWeights";
constexpr auto kAgingIntervalInMsecs = "agingIntervalInMsecs";
constexpr auto kCollectorDstIp = "collectorDstIp";
constexpr auto kCollectorSrcPort = "collectorSrcPort";
constexpr auto kCollectorDstPort = "collectorDstPort";
constexpr auto kCollectorDscp = "collectorDscp";
constexpr auto kPpsToQcm = "ppsToQcm";
constexpr auto kCollectorSrcIp = "collectorSrcIp";
constexpr auto kMonitorQcmPortList = "monitorQcmPortList";
constexpr auto kPort2QosQueueIds = "port2QosQueueIds";
constexpr auto kMonitorQcmCfgPortsOnly = "monitorQcmCfgPortsOnly";
} // namespace

namespace facebook::fboss {

folly::dynamic QcmCfgFields::toFollyDynamic() const {
  folly::dynamic qcmCfg = folly::dynamic::object;

  qcmCfg[kNumFlowSamplesPerView] = static_cast<uint32_t>(numFlowSamplesPerView);
  qcmCfg[kFlowLimit] = static_cast<uint32_t>(flowLimit);
  qcmCfg[kNumFlowsClear] = static_cast<uint32_t>(numFlowsClear);
  qcmCfg[kScanIntervalInUsecs] = static_cast<uint32_t>(scanIntervalInUsecs);
  qcmCfg[kExportThreshold] = static_cast<uint32_t>(exportThreshold);
  qcmCfg[kMonitorQcmCfgPortsOnly] = static_cast<bool>(monitorQcmCfgPortsOnly);

  folly::dynamic flowWeightMap = folly::dynamic::object;
  for (const auto& weight : flowWeights) {
    flowWeightMap[folly::to<std::string>(static_cast<int>(weight.first))] =
        weight.second;
  }
  qcmCfg[kFlowWeights] = flowWeightMap;
  qcmCfg[kAgingIntervalInMsecs] = static_cast<uint32_t>(agingIntervalInMsecs);
  qcmCfg[kCollectorDstIp] = folly::IPAddress::networkToString(collectorDstIp);
  qcmCfg[kCollectorSrcPort] = static_cast<uint32_t>(collectorSrcPort);
  qcmCfg[kCollectorDstPort] = static_cast<uint32_t>(collectorDstPort);
  if (collectorDscp) {
    qcmCfg[kCollectorDscp] = collectorDscp.value();
  }
  if (ppsToQcm) {
    qcmCfg[kPpsToQcm] = ppsToQcm.value();
  }
  qcmCfg[kCollectorSrcIp] = folly::IPAddress::networkToString(collectorSrcIp);

  qcmCfg[kMonitorQcmPortList] = folly::dynamic::array;
  for (const auto& qcmPort : monitorQcmPortList) {
    qcmCfg[kMonitorQcmPortList].push_back(qcmPort);
  }
  folly::dynamic port2QosQueueMap = folly::dynamic::object;
  for (const auto& perPortQosQueueIds : port2QosQueueIds) {
    folly::dynamic qcmSet = folly::dynamic::array;
    for (const auto& qosQueueId : perPortQosQueueIds.second) {
      qcmSet.push_back(qosQueueId);
    }
    port2QosQueueMap[folly::to<std::string>(
        static_cast<int>(perPortQosQueueIds.first))] = qcmSet;
  }
  qcmCfg[kPort2QosQueueIds] = port2QosQueueMap;
  return qcmCfg;
}

QcmCfgFields QcmCfgFields::fromFollyDynamic(const folly::dynamic& json) {
  QcmCfgFields qcmCfgFields = QcmCfgFields();

  if (json.find(kNumFlowSamplesPerView) != json.items().end()) {
    qcmCfgFields.numFlowSamplesPerView = json[kNumFlowSamplesPerView].asInt();
  }
  if (json.find(kFlowLimit) != json.items().end()) {
    qcmCfgFields.flowLimit = json[kFlowLimit].asInt();
  }
  if (json.find(kNumFlowsClear) != json.items().end()) {
    qcmCfgFields.numFlowsClear = json[kNumFlowsClear].asInt();
  }
  if (json.find(kScanIntervalInUsecs) != json.items().end()) {
    qcmCfgFields.scanIntervalInUsecs = json[kScanIntervalInUsecs].asInt();
  }
  if (json.find(kExportThreshold) != json.items().end()) {
    qcmCfgFields.exportThreshold = json[kExportThreshold].asInt();
  }
  if (json.find(kMonitorQcmCfgPortsOnly) != json.items().end()) {
    qcmCfgFields.monitorQcmCfgPortsOnly =
        json[kMonitorQcmCfgPortsOnly].asBool();
  }

  for (const auto& weight : json[kFlowWeights].items()) {
    qcmCfgFields.flowWeights[weight.first.asInt()] = weight.second.asInt();
  }
  if (json.find(kAgingIntervalInMsecs) != json.items().end()) {
    qcmCfgFields.agingIntervalInMsecs = json[kAgingIntervalInMsecs].asInt();
  }
  if (json.find(kCollectorDstIp) != json.items().end()) {
    qcmCfgFields.collectorDstIp =
        folly::IPAddress::createNetwork(json[kCollectorDstIp].asString());
  }
  if (json.find(kCollectorSrcPort) != json.items().end()) {
    qcmCfgFields.collectorSrcPort = json[kCollectorSrcPort].asInt();
  }
  if (json.find(kCollectorDstPort) != json.items().end()) {
    qcmCfgFields.collectorDstPort = json[kCollectorDstPort].asInt();
  }
  if (json.find(kCollectorDscp) != json.items().end()) {
    qcmCfgFields.collectorDscp = json[kCollectorDscp].asInt();
  }
  if (json.find(kPpsToQcm) != json.items().end()) {
    qcmCfgFields.ppsToQcm = json[kPpsToQcm].asInt();
  }
  if (json.find(kCollectorSrcIp) != json.items().end()) {
    qcmCfgFields.collectorSrcIp =
        folly::IPAddress::createNetwork(json[kCollectorSrcIp].asString());
  }

  if (json.find(kMonitorQcmPortList) != json.items().end()) {
    for (const auto& qcmPort : json[kMonitorQcmPortList]) {
      uint32_t port = qcmPort.asInt();
      qcmCfgFields.monitorQcmPortList.push_back(port);
    }
  }

  if (json.find(kPort2QosQueueIds) != json.items().end()) {
    for (const auto& perPortQosQueueIds : json[kPort2QosQueueIds].items()) {
      for (const auto& queueId : perPortQosQueueIds.second) {
        qcmCfgFields.port2QosQueueIds[perPortQosQueueIds.first.asInt()].insert(
            queueId.asInt());
      }
    }
  }
  return qcmCfgFields;
}

template class NodeBaseT<QcmCfg, QcmCfgFields>;

} // namespace facebook::fboss
