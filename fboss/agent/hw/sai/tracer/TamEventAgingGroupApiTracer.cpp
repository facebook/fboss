/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/TamEventAgingGroupApiTracer.h" // NOLINT(facebook-unused-include-check)

#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/TamEventAgingGroupApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)

using folly::to;

namespace {

std::map<int32_t, std::pair<std::string, std::size_t>> _TamEventAgingGroupMap{
    SAI_ATTR_MAP(TamEventAgingGroup, Type),
    SAI_ATTR_MAP(TamEventAgingGroup, AgingTime),
};

void handleExtensionAttributes() {}

} // namespace

namespace facebook::fboss {

WRAP_CREATE_FUNC(
    tam_event_aging_group,
    static_cast<sai_object_type_t>(SAI_OBJECT_TYPE_TAM_EVENT_AGING_GROUP),
    tamEventAgingGroup);
WRAP_REMOVE_FUNC(
    tam_event_aging_group,
    static_cast<sai_object_type_t>(SAI_OBJECT_TYPE_TAM_EVENT_AGING_GROUP),
    tamEventAgingGroup);
WRAP_SET_ATTR_FUNC(
    tam_event_aging_group,
    static_cast<sai_object_type_t>(SAI_OBJECT_TYPE_TAM_EVENT_AGING_GROUP),
    tamEventAgingGroup);
WRAP_GET_ATTR_FUNC(
    tam_event_aging_group,
    static_cast<sai_object_type_t>(SAI_OBJECT_TYPE_TAM_EVENT_AGING_GROUP),
    tamEventAgingGroup);

sai_tam_event_aging_group_api_t* wrappedTamEventAgingGroupApi() {
  handleExtensionAttributes();
  static sai_tam_event_aging_group_api_t tamEventAgingGroupWrappers;

  tamEventAgingGroupWrappers.create_tam_event_aging_group =
      &wrap_create_tam_event_aging_group;
  tamEventAgingGroupWrappers.remove_tam_event_aging_group =
      &wrap_remove_tam_event_aging_group;
  tamEventAgingGroupWrappers.set_tam_event_aging_group_attribute =
      &wrap_set_tam_event_aging_group_attribute;
  tamEventAgingGroupWrappers.get_tam_event_aging_group_attribute =
      &wrap_get_tam_event_aging_group_attribute;

  return &tamEventAgingGroupWrappers;
}

SET_SAI_ATTRIBUTES(TamEventAgingGroup)

} // namespace facebook::fboss

#endif
