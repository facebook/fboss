// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/TamApi.h"

extern "C" {
#if defined(TAJO_SAI_SDK)
#include <saiextensions.h>
#else
#include <experimental/sai_attr_ext.h>
#endif
}

namespace facebook::fboss {

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributeSwitchEventType::operator()() {
  return SAI_TAM_EVENT_ATTR_SWITCH_EVENT_TYPE;
}

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributeDeviceId::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributeEventId::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributeExtensionsCollectorList::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributePacketDropTypeMmu::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributeAgingGroup::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiTamTransportTraits::Attributes::AttributeSrcMacAddress::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiTamTransportTraits::Attributes::AttributeDstMacAddress::operator()() {
  return std::nullopt;
}

} // namespace facebook::fboss
