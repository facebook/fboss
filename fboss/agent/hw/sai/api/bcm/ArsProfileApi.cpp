/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/ArsProfileApi.h"

extern "C" {
#include <sai.h>

#if defined(BRCM_SAI_SDK_GTE_13_0) && defined(BRCM_SAI_SDK_XGS)
#ifndef IS_OSS_BRCM_SAI
#include <experimental/saiarsprofileextensions.h>
#else
#include <saiarsprofileextensions.h>
#endif
#endif
}

namespace facebook::fboss {

#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
std::optional<sai_attr_id_t> SaiArsProfileTraits::Attributes::
    AttributeExtensionSamplingIntervalNanosec::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0) && !defined(BRCM_SAI_SDK_GTE_14_0) && \
    defined(BRCM_SAI_SDK_XGS)
  return SAI_ARS_PROFILE_ATTR_EXTENSION_SAMPLING_INTERVAL_NANOSEC;
#else
  return std::nullopt;
#endif
}
#endif

#if SAI_API_VERSION >= SAI_VERSION(1, 16, 0)
std::optional<sai_attr_id_t>
SaiArsProfileTraits::Attributes::AttributeArsMaxGroups::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0) && defined(BRCM_SAI_SDK_XGS)
  return SAI_ARS_PROFILE_ATTR_EXTENSION_ECMP_ARS_MAX_GROUPS;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiArsProfileTraits::Attributes::AttributeArsBaseIndex::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0) && defined(BRCM_SAI_SDK_XGS)
  return SAI_ARS_PROFILE_ATTR_EXTENSION_ECMP_ARS_BASE_INDEX;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t> SaiArsProfileTraits::Attributes::
    AttributeArsAlternateMembersRouteMetaData::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0) && defined(BRCM_SAI_SDK_XGS)
  return SAI_ARS_PROFILE_ATTR_ROUTE_ARS_ALTERNATE_MEMBERS_META_DATA;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiArsProfileTraits::Attributes::AttributeArsRouteMetaDataMask::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0) && defined(BRCM_SAI_SDK_XGS)
  return SAI_ARS_PROFILE_ATTR_ROUTE_ARS_META_DATA_MASK;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t> SaiArsProfileTraits::Attributes::
    AttributeArsPrimaryMembersRouteMetaData::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0) && defined(BRCM_SAI_SDK_XGS)
  return SAI_ARS_PROFILE_ATTR_ROUTE_ARS_PRIMARY_MEMBERS_META_DATA;
#else
  return std::nullopt;
#endif
}
#endif

} // namespace facebook::fboss
