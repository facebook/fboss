// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/lib/QsfpConfigParserHelper.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_service_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

namespace facebook {
namespace fboss {

bool overrideFactorMatchFound(
    const cfg::TransceiverConfigOverrideFactor& cfgFactor,
    const cfg::TransceiverConfigOverrideFactor& moduleFactor) {
  // Returns true when all the set factors match the factors passed in the args
  bool matchFound = true;
  if (auto cfgFactorAppCode = cfgFactor.applicationCode()) {
    // Confirm that the appCode passed in the args is set and matches that in
    // config
    auto moduleAppCode = moduleFactor.applicationCode();
    matchFound &= (moduleAppCode && (*cfgFactorAppCode == *moduleAppCode));
  }
  if (auto cfgPartNumber = cfgFactor.transceiverPartNumber()) {
    // Confirm that the partNumber passed in the args is set and matches that in
    // config
    auto modulePartNumber = moduleFactor.transceiverPartNumber();
    matchFound &= (modulePartNumber && (*cfgPartNumber == *modulePartNumber));
  }
  return matchFound;
}

std::optional<RxEqualizerSettings> cmisRxEqualizerSettingOverride(
    const cfg::TransceiverOverrides& overrides) {
  if (auto cmisOverride = overrides.cmis_ref()) {
    if (auto rxEqSetting = cmisOverride->rxEqualizerSettings()) {
      return *rxEqSetting;
    }
  }
  return std::nullopt;
}

std::optional<unsigned int> sffRxPreemphasisOverride(
    const cfg::TransceiverOverrides& overrides) {
  if (auto sffOverride = overrides.sff_ref()) {
    if (auto preemph = sffOverride->rxPreemphasis()) {
      return *preemph;
    }
  }
  return std::nullopt;
}

std::optional<unsigned int> sffRxAmplitudeOverride(
    const cfg::TransceiverOverrides& overrides) {
  if (auto sffOverride = overrides.sff_ref()) {
    if (auto rxamp = sffOverride->rxAmplitude()) {
      return *rxamp;
    }
  }
  return std::nullopt;
}

std::optional<unsigned int> sffTxEqualizationOverride(
    const cfg::TransceiverOverrides& overrides) {
  if (auto sffOverride = overrides.sff_ref()) {
    if (auto txeq = sffOverride->txEqualization()) {
      return *txeq;
    }
  }
  return std::nullopt;
}

cfg::TransceiverConfigOverrideFactor getModuleConfigOverrideFactor(
    std::optional<cfg::TransceiverPartNumber> partNumber,
    std::optional<SMFMediaInterfaceCode> applicationCode) {
  cfg::TransceiverConfigOverrideFactor moduleFactor;
  if (partNumber) {
    moduleFactor.transceiverPartNumber() = *partNumber;
  }
  if (applicationCode) {
    moduleFactor.applicationCode() = *applicationCode;
  }
  return moduleFactor;
}

} // namespace fboss
} // namespace facebook
