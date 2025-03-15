// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/TamApi.h"

extern "C" {
#ifndef IS_OSS_BRCM_SAI
#include <experimental/saitamextensions.h>
#else
#include <saitamextensions.h>
#endif
}

namespace facebook::fboss {

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributeSwitchEventType::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributeDeviceId::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_TAM_EVENT_ATTR_DEVICE_ID;
#endif
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributeEventId::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_TAM_EVENT_ATTR_EVENT_ID;
#endif
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributeExtensionsCollectorList::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_TAM_EVENT_ATTR_EXTENSIONS_COLLECTOR_LIST;
#endif
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributePacketDropTypeMmu::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_TAM_EVENT_ATTR_PACKET_DROP_TYPE_MMU;
#endif
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributeAgingGroup::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_TAM_EVENT_ATTR_AGING_GROUP;
#endif
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiTamTransportTraits::Attributes::AttributeSrcMacAddress::operator()() {
  return SAI_TAM_TRANSPORT_ATTR_SRC_MAC_ADDRESS;
}

std::optional<sai_attr_id_t>
SaiTamTransportTraits::Attributes::AttributeDstMacAddress::operator()() {
  return SAI_TAM_TRANSPORT_ATTR_DST_MAC_ADDRESS;
}

} // namespace facebook::fboss
