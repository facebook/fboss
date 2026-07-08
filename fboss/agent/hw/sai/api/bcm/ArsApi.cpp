// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/ArsApi.h"

#if defined(BRCM_SAI_SDK_GTE_14_0)
#include <experimental/saiarsextensions.h>
#endif

namespace facebook::fboss {

#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
std::optional<sai_attr_id_t>
SaiArsTraits::Attributes::AttributeNextHopGroupType::operator()() {
#if defined(BRCM_SAI_SDK_GTE_14_0) && defined(BRCM_SAI_SDK_XGS)
  return SAI_ARS_ATTR_EXTENSION_NEXT_HOP_GROUP_TYPE;
#else
  return std::nullopt;
#endif
}
#endif

} // namespace facebook::fboss
