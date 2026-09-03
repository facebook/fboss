// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/AclApi.h"

extern "C" {
#if defined(TAJO_SDK_GTE_26_5)
#include <saiextensions.h>
#endif
}

namespace facebook::fboss {

std::optional<sai_attr_id_t>
SaiAclEntryTraits::Attributes::AttributeActionL3SwitchCancel::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiAclEntryTraits::Attributes::AttributeFieldRouteDestination::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiAclEntryTraits::Attributes::AttributeLabelExtendedWrapper::operator()() {
#if defined(TAJO_SDK_GTE_26_5) && !defined(TAJO_SDK_VERSION_26_5_5211) && \
    defined(SAI_ACL_ENTRY_ATTR_EXT_LABEL_EXTENDED)
  return SAI_ACL_ENTRY_ATTR_EXT_LABEL_EXTENDED;
#else
  return std::nullopt;
#endif
}

} // namespace facebook::fboss
