// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/SwitchApi.h"

extern "C" {
#include <sai.h>

#ifndef IS_OSS_BRCM_SAI
#include <experimental/saiswitchextensions.h>
#else
#include <saiswitchextensions.h>
#endif
}

namespace facebook::fboss {

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeMaxCoresWrapper::operator()() {
#if defined(BRCM_SAI_SDK_DNX)
  return SAI_SWITCH_ATTR_MAX_CORES;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeLedIdWrapper::operator()() {
  return SAI_SWITCH_ATTR_LED;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeLedResetIdWrapper::operator()() {
  return SAI_SWITCH_ATTR_LED_PROCESSOR_RESET;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeAclFieldListWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeEgressPoolAvaialableSizeIdWrapper::operator()() {
  return SAI_SWITCH_ATTR_DEFAULT_EGRESS_BUFFER_POOL_SHARED_SIZE;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::HwEccErrorInitiateWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeDllPathWrapper::operator()() {
#if defined(BRCM_SAI_SDK_XGS)
  return SAI_SWITCH_ATTR_ISSU_CUSTOM_DLL_PATH;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeRestartIssuWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeWarmBootTargetVersionWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeSdkBootTimeWrapper::operator()() {
  return SAI_SWITCH_ATTR_CUSTOM_RANGE_START + 1;
}

const std::vector<sai_stat_id_t>& SaiSwitchTraits::dramStats() {
#if defined(BRCM_SAI_SDK_DNX)
  static const std::vector<sai_stat_id_t> stats{
      SAI_SWITCH_STAT_DEVICE_DRAM_ENQUEUED_BYTES,
      SAI_SWITCH_STAT_DEVICE_DRAM_DEQUEUED_BYTES};
#else
  static const std::vector<sai_stat_id_t> stats;
#endif
  return stats;
}

const std::vector<sai_stat_id_t>& SaiSwitchTraits::rciWatermarkStats() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  static const std::vector<sai_stat_id_t> stats{
      SAI_SWITCH_STAT_DEVICE_RCI_WATERMARK_BYTES,
      SAI_SWITCH_STAT_DEVICE_CORE_RCI_WATERMARK_BYTES};
#else
  static const std::vector<sai_stat_id_t> stats;
#endif
  return stats;
}

const std::vector<sai_stat_id_t>& SaiSwitchTraits::dtlWatermarkStats() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  static const std::vector<sai_stat_id_t> stats{
      SAI_SWITCH_STAT_HIGHEST_QUEUE_CONGESTION_LEVEL};
#else
  static const std::vector<sai_stat_id_t> stats;
#endif
  return stats;
}

const std::vector<sai_stat_id_t>& SaiSwitchTraits::dramBlockTime() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  static const std::vector<sai_stat_id_t> stats{
      SAI_SWITCH_STAT_DEVICE_DRAM_BLOCK_TOTAL_TIME};
#else
  static const std::vector<sai_stat_id_t> stats;
#endif
  return stats;
}

const std::vector<sai_stat_id_t>&
SaiSwitchTraits::egressCoreBufferWatermarkBytes() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  static const std::vector<sai_stat_id_t> stats{
      SAI_SWITCH_STAT_DEVICE_EGRESS_DB_WM};
#else
  static const std::vector<sai_stat_id_t> stats;
#endif
  return stats;
}

const std::vector<sai_stat_id_t>& SaiSwitchTraits::deletedCredits() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  static const std::vector<sai_stat_id_t> stats{
      SAI_SWITCH_STAT_DEVICE_DELETED_CREDIT_COUNTER};
#else
  static const std::vector<sai_stat_id_t> stats;
#endif
  return stats;
}

void SwitchApi::registerSwitchEventCallback(
    SwitchSaiId id,
    void* switch_event_cb) const {
#if defined(BRCM_SAI_SDK_XGS_AND_DNX)
  sai_attribute_t attr;
  attr.value.ptr = switch_event_cb;
  attr.id = SAI_SWITCH_ATTR_SWITCH_EVENT_NOTIFY;
  sai_attribute_t eventAttr;
  eventAttr.id = SAI_SWITCH_ATTR_SWITCH_EVENT_TYPE;
  if (switch_event_cb) {
    // Register switch callback function
    auto rv = _setAttribute(id, &attr);
    saiLogError(
        rv, ApiType, "Unable to register parity error switch event callback");

    // Register switch events
#if defined BRCM_SAI_SDK_GTE_11_0
    std::array<uint32_t, 6> events = {
        SAI_SWITCH_EVENT_TYPE_PARITY_ERROR,
        SAI_SWITCH_EVENT_TYPE_STABLE_FULL,
        SAI_SWITCH_EVENT_TYPE_STABLE_ERROR,
        SAI_SWITCH_EVENT_TYPE_UNCONTROLLED_SHUTDOWN,
        SAI_SWITCH_EVENT_TYPE_WARM_BOOT_DOWNGRADE,
        SAI_SWITCH_EVENT_TYPE_INTERRUPT};
#else
    std::array<uint32_t, 5> events = {
        SAI_SWITCH_EVENT_TYPE_PARITY_ERROR,
        SAI_SWITCH_EVENT_TYPE_STABLE_FULL,
        SAI_SWITCH_EVENT_TYPE_STABLE_ERROR,
        SAI_SWITCH_EVENT_TYPE_UNCONTROLLED_SHUTDOWN,
        SAI_SWITCH_EVENT_TYPE_WARM_BOOT_DOWNGRADE};
#endif
    eventAttr.value.u32list.count = events.size();
    eventAttr.value.u32list.list = events.data();
    rv = _setAttribute(id, &eventAttr);
    saiLogError(rv, ApiType, "Unable to register parity error switch events");
  } else {
    // First unregister switch events
    eventAttr.value.u32list.count = 0;
    auto rv = _setAttribute(id, &eventAttr);
    saiLogError(rv, ApiType, "Unable to unregister switch events");

    // Then unregister callback function
    rv = _setAttribute(id, &attr);
    saiLogError(rv, ApiType, "Unable to unregister TAM event callback");
  }
#endif
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeDelayDropCongThreshold::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeForceTrafficOverFabricWrapper::operator()() {
#if defined(BRCM_SAI_SDK_DNX)
  return SAI_SWITCH_ATTR_FORCE_TRAFFIC_OVER_FABRIC;
#endif
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeSwitchIsolateWrapper::operator()() {
#if defined(BRCM_SAI_SDK_DNX)
  return SAI_SWITCH_ATTR_SWITCH_ISOLATE;
#endif
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeFabricRemoteReachablePortList::operator()() {
#if defined(BRCM_SAI_SDK_DNX) && defined(BRCM_SAI_SDK_GTE_11_0)
  return SAI_SWITCH_ATTR_FABRIC_REMOTE_REACHABLE_PORT_LIST;
#endif
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeRouteNoImplicitMetaDataWrapper::operator()() {
#if defined(BRCM_SAI_SDK_GTE_11_0)
  return SAI_SWITCH_ATTR_ROUTE_NO_IMPLICIT_META_DATA;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeRouteAllowImplicitMetaDataWrapper::operator()() {
#if defined(BRCM_SAI_SDK_GTE_11_0)
  return SAI_SWITCH_ATTR_ROUTE_ALLOW_IMPLICIT_META_DATA;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeMultiStageLocalSwitchIdsWrapper::operator()() {
#if defined(BRCM_SAI_SDK_GTE_11_0)
  return SAI_SWITCH_ATTR_MULTI_STAGE_LOCAL_SWITCH_IDS;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeVoqLatencyMinLocalNs::operator()() {
#if defined(BRCM_SAI_SDK_DNX) && defined(BRCM_SAI_SDK_GTE_11_0)
  return SAI_SWITCH_ATTR_VOQ_LATENCY_MIN_LOCAL;
#endif
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeVoqLatencyMaxLocalNs::operator()() {
#if defined(BRCM_SAI_SDK_DNX) && defined(BRCM_SAI_SDK_GTE_11_0)
  return SAI_SWITCH_ATTR_VOQ_LATENCY_MAX_LOCAL;
#endif
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeVoqLatencyMinLevel1Ns::operator()() {
#if defined(BRCM_SAI_SDK_DNX) && defined(BRCM_SAI_SDK_GTE_11_0)
  return SAI_SWITCH_ATTR_VOQ_LATENCY_MIN_LEVEL_1;
#endif
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeVoqLatencyMaxLevel1Ns::operator()() {
#if defined(BRCM_SAI_SDK_DNX) && defined(BRCM_SAI_SDK_GTE_11_0)
  return SAI_SWITCH_ATTR_VOQ_LATENCY_MAX_LEVEL_1;
#endif
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeVoqLatencyMinLevel2Ns::operator()() {
#if defined(BRCM_SAI_SDK_DNX) && defined(BRCM_SAI_SDK_GTE_11_0)
  return SAI_SWITCH_ATTR_VOQ_LATENCY_MIN_LEVEL_2;
#endif
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeVoqLatencyMaxLevel2Ns::operator()() {
#if defined(BRCM_SAI_SDK_DNX) && defined(BRCM_SAI_SDK_GTE_11_0)
  return SAI_SWITCH_ATTR_VOQ_LATENCY_MAX_LEVEL_2;
#endif
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeReachabilityGroupList::operator()() {
#if defined(BRCM_SAI_SDK_DNX) && defined(BRCM_SAI_SDK_GTE_12_0)
  return SAI_SWITCH_ATTR_REACHABILITY_GROUP_LIST;
#endif
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeFabricLinkLayerFlowControlThreshold::operator()() {
#if defined(BRCM_SAI_SDK_DNX) && defined(BRCM_SAI_SDK_GTE_12_0)
  return SAI_SWITCH_ATTR_FABRIC_LLFC_THRESHOLD;
#endif
  return std::nullopt;
}
} // namespace facebook::fboss
