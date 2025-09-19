// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/NextHopGroupApi.h"

#if defined(BRCM_SAI_SDK_GTE_13_0)
#ifndef IS_OSS_BRCM_SAI
#include <experimental/sainexthopgroupextensions.h>
#else
#include <sainexthopgroupextensions.h>
#endif
#endif

namespace facebook::fboss {

std::optional<sai_attr_id_t> SaiNextHopGroupTraits::Attributes::
    AttributeArsNextHopGroupMetaData::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0) && !defined(BRCM_SAI_SDK_GTE_14_0) && \
    defined(BRCM_SAI_SDK_XGS)
  return SAI_NEXT_HOP_GROUP_ATTR_ARS_NEXT_HOP_GROUP_META_DATA;
#else
  return std::nullopt;
#endif
}

} // namespace facebook::fboss
