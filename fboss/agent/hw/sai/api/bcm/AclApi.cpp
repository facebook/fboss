// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/AclApi.h"

#ifndef IS_OSS_BRCM_SAI
#include <experimental/saiaclextensions.h>
#else
#include <saiaclextensions.h>
#endif

namespace facebook::fboss {

std::optional<sai_attr_id_t>
SaiAclEntryTraits::Attributes::AttributeActionL3SwitchCancel::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0) && defined(BRCM_SAI_SDK_XGS)
  return SAI_ACL_ENTRY_ATTR_ACTION_L3_SWITCH_CANCEL;
#else
  return std::nullopt;
#endif
}

} // namespace facebook::fboss
