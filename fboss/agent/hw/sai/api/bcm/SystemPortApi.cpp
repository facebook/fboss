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
// TODO(zecheng): Update flag when new 12.0 release has the attribute
#if defined(SAI_VERSION_11_7_0_0_DNX_ODP)
  return SAI_SYSTEM_PORT_ATTR_SHEL_PKT_DEST_ENABLE;
#else
  return std::nullopt;
#endif
}

} // namespace facebook::fboss
