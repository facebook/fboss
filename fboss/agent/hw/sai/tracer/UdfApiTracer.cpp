/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/UdfApiTracer.h"
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/UdfApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

namespace {
std::map<int32_t, std::pair<std::string, std::size_t>> _UdfMap{
    SAI_ATTR_MAP(Udf, UdfMatchId),
    SAI_ATTR_MAP(Udf, UdfGroupId),
    SAI_ATTR_MAP(Udf, Base),
    SAI_ATTR_MAP(Udf, Offset),
};

std::map<int32_t, std::pair<std::string, std::size_t>> _UdfMatchMap {
  SAI_ATTR_MAP(UdfMatch, L2Type), SAI_ATTR_MAP(UdfMatch, L3Type),
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
      SAI_ATTR_MAP(UdfMatch, L4DstPortType),
#endif
};

std::map<int32_t, std::pair<std::string, std::size_t>> _UdfGroupMap{
    SAI_ATTR_MAP(UdfGroup, Type),
    SAI_ATTR_MAP(UdfGroup, Length),
};

} // namespace

namespace facebook::fboss {

WRAP_CREATE_FUNC(udf, SAI_OBJECT_TYPE_UDF, udf);
WRAP_REMOVE_FUNC(udf, SAI_OBJECT_TYPE_UDF, udf);
WRAP_SET_ATTR_FUNC(udf, SAI_OBJECT_TYPE_UDF, udf);
WRAP_GET_ATTR_FUNC(udf, SAI_OBJECT_TYPE_UDF, udf);

WRAP_CREATE_FUNC(udf_match, SAI_OBJECT_TYPE_UDF_MATCH, udf);
WRAP_REMOVE_FUNC(udf_match, SAI_OBJECT_TYPE_UDF_MATCH, udf);
WRAP_SET_ATTR_FUNC(udf_match, SAI_OBJECT_TYPE_UDF_MATCH, udf);
WRAP_GET_ATTR_FUNC(udf_match, SAI_OBJECT_TYPE_UDF_MATCH, udf);

WRAP_CREATE_FUNC(udf_group, SAI_OBJECT_TYPE_UDF_GROUP, udf);
WRAP_REMOVE_FUNC(udf_group, SAI_OBJECT_TYPE_UDF_GROUP, udf);
WRAP_SET_ATTR_FUNC(udf_group, SAI_OBJECT_TYPE_UDF_GROUP, udf);
WRAP_GET_ATTR_FUNC(udf_group, SAI_OBJECT_TYPE_UDF_GROUP, udf);

sai_udf_api_t* wrappedUdfApi() {
  static sai_udf_api_t udfWrappers;

  udfWrappers.create_udf = &wrap_create_udf;
  udfWrappers.remove_udf = &wrap_remove_udf;
  udfWrappers.set_udf_attribute = &wrap_set_udf_attribute;
  udfWrappers.get_udf_attribute = &wrap_get_udf_attribute;
  udfWrappers.create_udf_match = &wrap_create_udf_match;
  udfWrappers.remove_udf_match = &wrap_remove_udf_match;
  udfWrappers.set_udf_match_attribute = &wrap_set_udf_match_attribute;
  udfWrappers.get_udf_match_attribute = &wrap_get_udf_match_attribute;
  udfWrappers.create_udf_group = &wrap_create_udf_group;
  udfWrappers.remove_udf_group = &wrap_remove_udf_group;
  udfWrappers.set_udf_group_attribute = &wrap_set_udf_group_attribute;
  udfWrappers.get_udf_group_attribute = &wrap_get_udf_group_attribute;

  return &udfWrappers;
}

SET_SAI_ATTRIBUTES(Udf)
SET_SAI_ATTRIBUTES(UdfMatch)
SET_SAI_ATTRIBUTES(UdfGroup)

} // namespace facebook::fboss
