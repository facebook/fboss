// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/SystemPortApi.h"

extern "C" {
#include <sai.h>

#ifndef IS_OSS_BRCM_SAI
#include <experimental/saiportextensions.h>
#else
#include <saiportextensions.h>
#endif
}

namespace facebook::fboss {

std::optional<sai_attr_id_t>
SaiSystemPortTraits::Attributes::AttributeShelPktDstEnable::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_SYSTEM_PORT_ATTR_SHEL_PKT_DEST_ENABLE;
#else
  return std::nullopt;
#endif
}

} // namespace facebook::fboss
