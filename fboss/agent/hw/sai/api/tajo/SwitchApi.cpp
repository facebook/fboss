// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/SwitchApi.h"

extern "C" {
#include <sai.h>

#include <experimental/sai_attr_ext.h>
}

namespace facebook::fboss {

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeLedIdWrapper::operator()() {
  return std::nullopt;
}
std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeLedResetIdWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeAclFieldListWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeEgressPoolAvaialableSizeIdWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::HwEccErrorInitiateWrapper::operator()() {
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
  return SAI_SWITCH_ATTR_EXT_HW_ECC_ERROR_INITIATE;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeDllPathWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeRestartIssuWrapper::operator()() {
#if defined(TAJO_P4_WB_SDK)
  return SAI_SWITCH_ATTR_EXT_RESTART_ISSU;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeDelayDropCongThreshold::operator()() {
#if defined(TAJO_SDK_VERSION_1_42_8)
  return SAI_SWITCH_ATTR_EXT_DELAY_DROP_CONG_THRESHOLD;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeForceTrafficOverFabricWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeWarmBootTargetVersionWrapper::operator()() {
#if defined(TAJO_SDK_VERSION_1_42_8)
  return SAI_SWITCH_ATTR_EXT_WARM_BOOT_TARGET_VERSION;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeSwitchIsolateWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeMaxCoresWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeSdkBootTimeWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeFabricRemoteReachablePortList::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeRouteNoImplicitMetaDataWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeRouteAllowImplicitMetaDataWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeMultiStageLocalSwitchIdsWrapper::operator()() {
  return std::nullopt;
}

const std::vector<sai_stat_id_t>& SaiSwitchTraits::dramStats() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>& SaiSwitchTraits::rciWatermarkStats() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>& SaiSwitchTraits::dtlWatermarkStats() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>& SaiSwitchTraits::dramBlockTime() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>& SaiSwitchTraits::deletedCredits() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>&
SaiSwitchTraits::egressCoreBufferWatermarkBytes() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

void SwitchApi::registerSwitchEventCallback(
    SwitchSaiId /*id*/,
    void* /*switch_event_cb*/) const {}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeVoqLatencyMinLocalNs::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeVoqLatencyMaxLocalNs::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeVoqLatencyMinLevel1Ns::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeVoqLatencyMaxLevel1Ns::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeVoqLatencyMinLevel2Ns::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeVoqLatencyMaxLevel2Ns::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeReachabilityGroupList::operator()() {
  return std::nullopt;
}
std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeFabricLinkLayerFlowControlThreshold::operator()() {
  return std::nullopt;
}
} // namespace facebook::fboss
