// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

#include "fboss/agent/hw/sai/api/TamApi.h"

extern "C" {
#include <experimental/saiexperimentalswitch.h>
#include <experimental/saitamextensions.h>
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
} // namespace

namespace facebook::fboss {

void SaiSwitch::tamEventCallback(
    sai_object_id_t /*tam_event_id*/,
    sai_size_t /*buffer_size*/,
    const void* /*buffer*/,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  SaiTamEventTraits::Attributes::SwitchEventId eventID{};

  for (auto i = 0; i < attr_count; i++) {
    if (attr_list[i].id == eventID.id()) {
      eventID =
          SaiTamEventTraits::Attributes::SwitchEventId(attr_list[i].value.u32);
    }
  }

  XLOG(WARNING) << "received switch event " << eventName(eventID.value());

  switch (eventID.value()) {
    case SAI_SWITCH_EVENT_TYPE_STABLE_FULL:
    case SAI_SWITCH_EVENT_TYPE_STABLE_ERROR:
    case SAI_SWITCH_EVENT_TYPE_UNCONTROLLED_SHUTDOWN:
    case SAI_SWITCH_EVENT_TYPE_WARM_BOOT_DOWNGRADE:
      getSwitchStats()->asicError();
      break;
    case SAI_SWITCH_EVENT_TYPE_PARITY_ERROR:
      // TODO: extend this when more information on fatality of event is
      // returned.
      getSwitchStats()->corrParityError();
      break;
  }
}
} // namespace facebook::fboss
