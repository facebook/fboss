// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

#include "fboss/agent/hw/sai/api/TamApi.h"

#include <folly/String.h>

extern "C" {
#if !defined(SAI_VERSION_7_2_0_0_ODP) && !defined(SAI_VERSION_8_2_0_0_ODP) && \
    !defined(SAI_VERSION_8_2_0_0_SIM_ODP) &&                                  \
    !defined(SAI_VERSION_8_2_0_0_DNX_ODP) &&                                  \
    !defined(SAI_VERSION_9_0_EA_SIM_ODP) &&                                   \
    !defined(SAI_VERSION_10_0_EA_DNX_SIM_ODP) &&                              \
    !defined(SAI_VERSION_9_0_EA_ODP) && !defined(SAI_VERSION_10_0_EA_DNX_ODP)
#include <experimental/saiexperimentalswitch.h>
#include <experimental/saitamextensions.h>
#else
#ifndef IS_OSS_BRCM_SAI
#include <experimental/saiswitchextensions.h>
#else
#include <saiswitchextensions.h>
#endif
#endif
}

namespace {
std::string eventName(uint32_t eventID) {
  switch (eventID) {
    case SAI_SWITCH_EVENT_TYPE_STABLE_FULL:
      return "SAI_SWITCH_EVENT_TYPE_STABLE_FULL";
    case SAI_SWITCH_EVENT_TYPE_STABLE_ERROR:
      return "SAI_SWITCH_EVENT_TYPE_STABLE_ERROR";
    case SAI_SWITCH_EVENT_TYPE_UNCONTROLLED_SHUTDOWN:
      return "SAI_SWITCH_EVENT_TYPE_UNCONTROLLED_SHUTDOWN";
    case SAI_SWITCH_EVENT_TYPE_WARM_BOOT_DOWNGRADE:
      return "SAI_SWITCH_EVENT_TYPE_UNCONTROLLED_SHUTDOWN";
    case SAI_SWITCH_EVENT_TYPE_PARITY_ERROR:
      return "SAI_SWITCH_EVENT_TYPE_PARITY_ERROR";
  }
  return folly::to<std::string>("unknown event type: ", eventID);
}

std::string correctionType(sai_switch_correction_type_t type) {
  switch (type) {
    case SAI_SWITCH_CORRECTION_TYPE_NO_ACTION:
      return "SAI_SWITCH_CORRECTION_TYPE_NO_ACTION";
    case SAI_SWITCH_CORRECTION_TYPE_FAIL_TO_CORRECT:
      return "SAI_SWITCH_CORRECTION_TYPE_FAIL_TO_CORRECT";
    case SAI_SWITCH_CORRECTION_TYPE_ENTRY_CLEAR:
      return "SAI_SWITCH_CORRECTION_TYPE_ENTRY_CLEAR";
    case SAI_SWITCH_CORRECTION_TYPE_CACHE_RESTORE:
      return "SAI_SWITCH_CORRECTION_TYPE_CACHE_RESTORE";
    case SAI_SWITCH_CORRECTION_TYPE_HW_CACHE_RESTORE:
      return "SAI_SWITCH_CORRECTION_TYPE_HW_CACHE_RESTORE";
    case SAI_SWITCH_CORRECTION_TYPE_SPECIAL:
      return "SAI_SWITCH_CORRECTION_TYPE_SPECIAL";
    case SAI_SWITCH_CORRECTION_TYPE_ALL:
      return "SAI_SWITCH_CORRECTION_TYPE_ALL";
  }
  return "correction-type-unknown";
}
} // namespace

namespace facebook::fboss {

void SaiSwitch::parityErrorSwitchEventCallback(
    sai_size_t /*buffer_size*/,
    const void* buffer,
    uint32_t event_type) {
  const sai_switch_ser_log_info_t* eventInfo =
      static_cast<const sai_switch_ser_log_info_t*>(buffer);
  std::stringstream sstream;
  sstream << "received switch event: " << eventName(event_type)
          << ", event info(";
  bool correctible = true;
  if (eventInfo) {
    sstream << "correction type=" << correctionType(eventInfo->correction_type)
            << " , flags=" << std::hex << eventInfo->flags;
    correctible =
        (eventInfo->correction_type !=
         SAI_SWITCH_CORRECTION_TYPE_FAIL_TO_CORRECT);
  }
  sstream << ")";
  XLOG(WARNING) << sstream.str();
  switch (event_type) {
    case SAI_SWITCH_EVENT_TYPE_STABLE_FULL:
    case SAI_SWITCH_EVENT_TYPE_STABLE_ERROR:
    case SAI_SWITCH_EVENT_TYPE_UNCONTROLLED_SHUTDOWN:
    case SAI_SWITCH_EVENT_TYPE_WARM_BOOT_DOWNGRADE:
      getSwitchStats()->asicError();
      break;
    case SAI_SWITCH_EVENT_TYPE_PARITY_ERROR:
      if (correctible) {
        getSwitchStats()->corrParityError();
      } else {
        getSwitchStats()->uncorrParityError();
      }
      break;
  }
}

void SaiSwitch::tamEventCallback(
    sai_object_id_t /*tam_event_id*/,
    sai_size_t /*buffer_size*/,
    const void* buffer,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  SaiTamEventTraits::Attributes::SwitchEventId eventID{};
  const sai_switch_ser_log_info_t* eventInfo =
      static_cast<const sai_switch_ser_log_info_t*>(buffer);
  for (auto i = 0; i < attr_count; i++) {
    if (attr_list[i].id == eventID.id()) {
      eventID =
          SaiTamEventTraits::Attributes::SwitchEventId(attr_list[i].value.u32);
    }
  }

  std::stringstream sstream;
  sstream << "received switch event: " << eventName(eventID.value())
          << ", event info(";
  bool correctible = true;
  if (eventInfo) {
    sstream << "correction type=" << correctionType(eventInfo->correction_type)
            << " , flags=" << std::hex << eventInfo->correction_type;
    correctible =
        (eventInfo->correction_type !=
         SAI_SWITCH_CORRECTION_TYPE_FAIL_TO_CORRECT);
  }
  sstream << ")";
  XLOG(WARNING) << sstream.str();
  switch (eventID.value()) {
    case SAI_SWITCH_EVENT_TYPE_STABLE_FULL:
    case SAI_SWITCH_EVENT_TYPE_STABLE_ERROR:
    case SAI_SWITCH_EVENT_TYPE_UNCONTROLLED_SHUTDOWN:
    case SAI_SWITCH_EVENT_TYPE_WARM_BOOT_DOWNGRADE:
      getSwitchStats()->asicError();
      break;
    case SAI_SWITCH_EVENT_TYPE_PARITY_ERROR:
      if (correctible) {
        getSwitchStats()->corrParityError();
      } else {
        getSwitchStats()->uncorrParityError();
      }
      break;
  }
}

} // namespace facebook::fboss
