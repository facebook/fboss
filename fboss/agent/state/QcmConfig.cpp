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

template class ThriftStructNode<QcmCfg, state::QcmCfgFields>;

} // namespace facebook::fboss
