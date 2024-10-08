// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/SwitchApi.h"

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
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeDllPathWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeRestartIssuWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeDelayDropCongThreshold::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeForceTrafficOverFabricWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeWarmBootTargetVersionWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeSwitchIsolateWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeMaxCoresWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeFabricRemoteReachablePortList::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeSdkBootTimeWrapper::operator()() {
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
