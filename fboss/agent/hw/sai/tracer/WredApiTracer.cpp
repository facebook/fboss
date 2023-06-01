/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/tracer/WredApiTracer.h"
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/WredApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

namespace {
std::map<int32_t, std::pair<std::string, std::size_t>> _WredMap{
    SAI_ATTR_MAP(Wred, GreenEnable),
    SAI_ATTR_MAP(Wred, GreenMinThreshold),
    SAI_ATTR_MAP(Wred, GreenMaxThreshold),
    SAI_ATTR_MAP(Wred, GreenDropProbability),
    SAI_ATTR_MAP(Wred, EcnMarkMode),
    SAI_ATTR_MAP(Wred, EcnGreenMinThreshold),
    SAI_ATTR_MAP(Wred, EcnGreenMaxThreshold),
};

} // namespace

namespace facebook::fboss {

WRAP_CREATE_FUNC(wred, SAI_OBJECT_TYPE_WRED, wred);
WRAP_REMOVE_FUNC(wred, SAI_OBJECT_TYPE_WRED, wred);
WRAP_SET_ATTR_FUNC(wred, SAI_OBJECT_TYPE_WRED, wred);
WRAP_GET_ATTR_FUNC(wred, SAI_OBJECT_TYPE_WRED, wred);

sai_wred_api_t* wrappedWredApi() {
  static sai_wred_api_t WredWrappers;

  WredWrappers.create_wred = &wrap_create_wred;
  WredWrappers.remove_wred = &wrap_remove_wred;
  WredWrappers.set_wred_attribute = &wrap_set_wred_attribute;
  WredWrappers.get_wred_attribute = &wrap_get_wred_attribute;

  return &WredWrappers;
}

SET_SAI_ATTRIBUTES(Wred)

} // namespace facebook::fboss
