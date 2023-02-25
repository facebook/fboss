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
#if defined(SAI_VERSION_8_2_0_0_ODP) ||                                        \
    defined(SAI_VERSION_8_2_0_0_SIM_ODP) || defined(SAI_VERSION_9_0_EA_ODP) || \
    defined(SAI_VERSION_9_0_EA_SIM_ODP)
  return SAI_SWITCH_ATTR_ISSU_CUSTOM_DLL_PATH;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeRestartIssuWrapper::operator()() {
  return std::nullopt;
}

void SwitchApi::registerParityErrorSwitchEventCallback(
    SwitchSaiId id,
    void* switch_event_cb) const {
#if defined(SAI_VERSION_7_2_0_0_ODP) || defined(SAI_VERSION_8_2_0_0_ODP) ||    \
    defined(SAI_VERSION_8_2_0_0_SIM_ODP) ||                                    \
    defined(SAI_VERSION_8_2_0_0_DNX_ODP) || defined(SAI_VERSION_9_0_EA_ODP) || \
    defined(SAI_VERSION_9_0_EA_DNX_ODP) || defined(SAI_VERSION_9_0_EA_SIM_ODP)
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
    std::array<uint32_t, 5> events = {
        SAI_SWITCH_EVENT_TYPE_PARITY_ERROR,
        SAI_SWITCH_EVENT_TYPE_STABLE_FULL,
        SAI_SWITCH_EVENT_TYPE_STABLE_ERROR,
        SAI_SWITCH_EVENT_TYPE_UNCONTROLLED_SHUTDOWN,
        SAI_SWITCH_EVENT_TYPE_WARM_BOOT_DOWNGRADE};
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

} // namespace facebook::fboss
