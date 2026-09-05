// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include "fboss/agent/hw/HwSwitchFb303Stats.h"

#include <folly/String.h>
#include <folly/logging/xlog.h>

extern "C" {
#include <experimental/saiswitchextensions.h>
#if defined(BRCM_SAI_SDK_XGS_GTE_15_0)
#include <experimental/saidropextensions.h>
#endif
}

namespace facebook::fboss {

void publishSwitchWatermarks(HwSwitchWatermarkStats& /*watermarkStats*/) {}

void publishSwitchPipelineStats(HwSwitchPipelineStats& /*pipelineStats*/) {}

void publishSwitchTemperatureStats(
    HwSwitchTemperatureStats& /*temperatureStats*/) {}

void logDropReasons(
    const std::vector<sai_int32_t>& ingressDropReasons,
    const std::vector<sai_int32_t>& egressDropReasons) {
#if defined(BRCM_SAI_SDK_XGS_GTE_15_0)
  std::vector<std::string> ingressNames;
  for (auto reason : ingressDropReasons) {
    switch (static_cast<sai_packet_drop_type_ingress_t>(reason)) {
      case SAI_PACKET_DROP_TYPE_INGRESS_L3_DST_DISCARD:
        ingressNames.push_back("L3_DST_DISCARD");
        break;
      case SAI_PACKET_DROP_TYPE_INGRESS_L3_TTL_ERROR:
        ingressNames.push_back("L3_TTL_ERROR");
        break;
      case SAI_PACKET_DROP_TYPE_INGRESS_SRC_ROUTE_DROP:
        ingressNames.push_back("SRC_ROUTE_DROP");
        break;
      case SAI_PACKET_DROP_TYPE_INGRESS_RFILDR:
        ingressNames.push_back("RFILDR");
        break;
      case SAI_PACKET_DROP_TYPE_INGRESS_SRC_PORT_KNOCKOUT_DROP:
        ingressNames.push_back("SRC_PORT_KNOCKOUT_DROP");
        break;
      case SAI_PACKET_DROP_TYPE_INGRESS_RDISC:
        ingressNames.push_back("RDISC");
        break;
      case SAI_PACKET_DROP_TYPE_INGRESS_RIMDR:
        ingressNames.push_back("RIMDR");
        break;
      case SAI_PACKET_DROP_TYPE_INGRESS_RDROP:
        ingressNames.push_back("RDROP");
        break;
      case SAI_PACKET_DROP_TYPE_INGRESS_RUC:
        ingressNames.push_back("RUC");
        break;
      default:
        ingressNames.push_back(std::to_string(reason));
        break;
    }
  }
  if (!ingressNames.empty()) {
    XLOG(INFO) << "DROP reasons ingress: " << folly::join(", ", ingressNames);
  }

  std::vector<std::string> egressNames;
  for (auto reason : egressDropReasons) {
    switch (static_cast<sai_packet_drop_type_egress_t>(reason)) {
      case SAI_PACKET_DROP_TYPE_EGRESS_TL2_MTU:
        egressNames.push_back("TL2_MTU");
        break;
      default:
        egressNames.push_back(std::to_string(reason));
        break;
    }
  }
  if (!egressNames.empty()) {
    XLOG(INFO) << "DROP reasons egress: " << folly::join(", ", egressNames);
  }
#endif
}

} // namespace facebook::fboss
