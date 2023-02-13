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
#include <fboss/agent/gen-cpp2/switch_state_types.h>
#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <fboss/agent/state/Thrifty.h>
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/SwitchState.h"

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

folly::dynamic QcmCfgFields::toFollyDynamicLegacy() const {
  folly::dynamic qcmCfg = folly::dynamic::object;

  qcmCfg[kNumFlowSamplesPerView] =
      static_cast<uint32_t>(numFlowSamplesPerView());
  qcmCfg[kFlowLimit] = static_cast<uint32_t>(flowLimit());
  qcmCfg[kNumFlowsClear] = static_cast<uint32_t>(numFlowsClear());
  qcmCfg[kScanIntervalInUsecs] = static_cast<uint32_t>(scanIntervalInUsecs());
  qcmCfg[kExportThreshold] = static_cast<uint32_t>(exportThreshold());
  qcmCfg[kMonitorQcmCfgPortsOnly] = static_cast<bool>(monitorQcmCfgPortsOnly());

  folly::dynamic flowWeightMap = folly::dynamic::object;
  for (const auto& weight : flowWeights()) {
    flowWeightMap[folly::to<std::string>(static_cast<int>(weight.first))] =
        weight.second;
  }
  qcmCfg[kFlowWeights] = flowWeightMap;
  qcmCfg[kAgingIntervalInMsecs] = static_cast<uint32_t>(agingIntervalInMsecs());
  qcmCfg[kCollectorDstIp] = folly::IPAddress::networkToString(collectorDstIp());
  qcmCfg[kCollectorSrcPort] = static_cast<uint32_t>(collectorSrcPort());
  qcmCfg[kCollectorDstPort] = static_cast<uint32_t>(collectorDstPort());
  if (auto dscp = collectorDscp()) {
    qcmCfg[kCollectorDscp] = dscp.value();
  }
  if (auto pps2qcm = ppsToQcm()) {
    qcmCfg[kPpsToQcm] = pps2qcm.value();
  }
  qcmCfg[kCollectorSrcIp] = folly::IPAddress::networkToString(collectorSrcIp());

  qcmCfg[kMonitorQcmPortList] = folly::dynamic::array;
  for (const auto& qcmPort : monitorQcmPortList()) {
    qcmCfg[kMonitorQcmPortList].push_back(qcmPort);
  }
  folly::dynamic port2QosQueueMap = folly::dynamic::object;
  for (const auto& perPortQosQueueIds : port2QosQueueIds()) {
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

QcmCfgFields QcmCfgFields::fromFollyDynamicLegacy(const folly::dynamic& json) {
  QcmCfgFields qcmCfgFields = QcmCfgFields();

  if (json.find(kNumFlowSamplesPerView) != json.items().end()) {
    qcmCfgFields.setNumFlowSamplesPerView(json[kNumFlowSamplesPerView].asInt());
  }
  if (json.find(kFlowLimit) != json.items().end()) {
    qcmCfgFields.setFlowLimit(json[kFlowLimit].asInt());
  }
  if (json.find(kNumFlowsClear) != json.items().end()) {
    qcmCfgFields.setNumFlowsClear(json[kNumFlowsClear].asInt());
  }
  if (json.find(kScanIntervalInUsecs) != json.items().end()) {
    qcmCfgFields.setScanIntervalInUsecs(json[kScanIntervalInUsecs].asInt());
  }
  if (json.find(kExportThreshold) != json.items().end()) {
    qcmCfgFields.setExportThreshold(json[kExportThreshold].asInt());
  }
  if (json.find(kMonitorQcmCfgPortsOnly) != json.items().end()) {
    qcmCfgFields.setMonitorQcmCfgPortsOnly(
        json[kMonitorQcmCfgPortsOnly].asBool());
  }

  WeightMap weightMap;
  for (const auto& weight : json[kFlowWeights].items()) {
    weightMap[weight.first.asInt()] = weight.second.asInt();
  }
  qcmCfgFields.setFlowWeights(std::move(weightMap));
  if (json.find(kAgingIntervalInMsecs) != json.items().end()) {
    qcmCfgFields.setAgingIntervalInMsecs(json[kAgingIntervalInMsecs].asInt());
  }
  if (json.find(kCollectorDstIp) != json.items().end()) {
    qcmCfgFields.setCollectorDstIp(
        folly::IPAddress::createNetwork(json[kCollectorDstIp].asString()));
  }
  if (json.find(kCollectorSrcPort) != json.items().end()) {
    qcmCfgFields.setCollectorSrcPort(json[kCollectorSrcPort].asInt());
  }
  if (json.find(kCollectorDstPort) != json.items().end()) {
    qcmCfgFields.setCollectorDstPort(json[kCollectorDstPort].asInt());
  }
  if (json.find(kCollectorDscp) != json.items().end()) {
    qcmCfgFields.setCollectorDscp(json[kCollectorDscp].asInt());
  }
  if (json.find(kPpsToQcm) != json.items().end()) {
    qcmCfgFields.setPpsToQcm(json[kPpsToQcm].asInt());
  }
  if (json.find(kCollectorSrcIp) != json.items().end()) {
    qcmCfgFields.setCollectorSrcIp(
        folly::IPAddress::createNetwork(json[kCollectorSrcIp].asString()));
  }

  std::vector<int32_t> monitorQcmPortList{};
  if (json.find(kMonitorQcmPortList) != json.items().end()) {
    for (const auto& qcmPort : json[kMonitorQcmPortList]) {
      uint32_t port = qcmPort.asInt();
      monitorQcmPortList.push_back(port);
    }
  }
  qcmCfgFields.setMonitorQcmPortList(std::move(monitorQcmPortList));

  if (json.find(kPort2QosQueueIds) != json.items().end()) {
    Port2QosQueueIdMap port2QosQueueIds;
    for (const auto& perPortQosQueueIds : json[kPort2QosQueueIds].items()) {
      for (const auto& queueId : perPortQosQueueIds.second) {
        port2QosQueueIds[perPortQosQueueIds.first.asInt()].insert(
            queueId.asInt());
      }
    }
    qcmCfgFields.setPort2QosQueueIds(std::move(port2QosQueueIds));
  }
  return qcmCfgFields;
}

template class ThriftStructNode<QcmCfg, state::QcmCfgFields>;

} // namespace facebook::fboss
