// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/HostifApi.h"

extern "C" {
#include <sai.h>

#ifndef IS_OSS_BRCM_SAI
#include <experimental/saihostifextensions.h>
#else
#include <saihostifextensions.h>
#endif
}

namespace facebook::fboss {

std::optional<sai_attr_id_t>
SaiTxPacketTraits::Attributes::AttributePacketType::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_13_0)
  return SAI_HOSTIF_PACKET_ATTR_PACKET_TYPE;
#else
  return std::nullopt;
#endif
}

} // namespace facebook::fboss
