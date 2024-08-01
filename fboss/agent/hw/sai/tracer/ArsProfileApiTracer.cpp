/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/ArsProfileApiTracer.h"
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/ArsProfileApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

namespace {
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
std::map<int32_t, std::pair<std::string, std::size_t>> _ArsProfileMap{
    SAI_ATTR_MAP(ArsProfile, Algo),
    SAI_ATTR_MAP(ArsProfile, SamplingInterval),
    SAI_ATTR_MAP(ArsProfile, RandomSeed),
    SAI_ATTR_MAP(ArsProfile, EnableIPv4),
    SAI_ATTR_MAP(ArsProfile, EnableIPv6),
    SAI_ATTR_MAP(ArsProfile, PortLoadPast),
    SAI_ATTR_MAP(ArsProfile, PortLoadPastWeight),
    SAI_ATTR_MAP(ArsProfile, LoadPastMinVal),
    SAI_ATTR_MAP(ArsProfile, LoadPastMaxVal),
    SAI_ATTR_MAP(ArsProfile, PortLoadFuture),
    SAI_ATTR_MAP(ArsProfile, PortLoadFutureWeight),
    SAI_ATTR_MAP(ArsProfile, LoadFutureMinVal),
    SAI_ATTR_MAP(ArsProfile, LoadFutureMaxVal),
    SAI_ATTR_MAP(ArsProfile, PortLoadCurrent),
    SAI_ATTR_MAP(ArsProfile, PortLoadExponent),
    SAI_ATTR_MAP(ArsProfile, LoadCurrentMinVal),
    SAI_ATTR_MAP(ArsProfile, LoadCurrentMaxVal),
    SAI_ATTR_MAP(ArsProfile, MaxFlows),
};
#endif

} // namespace

namespace facebook::fboss {

#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
WRAP_CREATE_FUNC(ars_profile, SAI_OBJECT_TYPE_ARS_PROFILE, arsProfile);
WRAP_REMOVE_FUNC(ars_profile, SAI_OBJECT_TYPE_ARS_PROFILE, arsProfile);
WRAP_SET_ATTR_FUNC(ars_profile, SAI_OBJECT_TYPE_ARS_PROFILE, arsProfile);
WRAP_GET_ATTR_FUNC(ars_profile, SAI_OBJECT_TYPE_ARS_PROFILE, arsProfile);

sai_ars_profile_api_t* wrappedArsProfileApi() {
  static sai_ars_profile_api_t arsProfileWrappers;

  arsProfileWrappers.create_ars_profile = &wrap_create_ars_profile;
  arsProfileWrappers.remove_ars_profile = &wrap_remove_ars_profile;
  arsProfileWrappers.set_ars_profile_attribute =
      &wrap_set_ars_profile_attribute;
  arsProfileWrappers.get_ars_profile_attribute =
      &wrap_get_ars_profile_attribute;

  return &arsProfileWrappers;
}

SET_SAI_ATTRIBUTES(ArsProfile)
#endif

} // namespace facebook::fboss
