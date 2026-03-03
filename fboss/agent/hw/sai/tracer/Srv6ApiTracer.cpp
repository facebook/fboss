// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/tracer/Srv6ApiTracer.h"
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/Srv6Api.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
namespace {
// NOLINTNEXTLINE(facebook-avoid-non-const-global-variables)
std::map<int32_t, std::pair<std::string, std::size_t>> _Srv6SidListMap{
    SAI_ATTR_MAP(Srv6SidList, Type),
    SAI_ATTR_MAP(Srv6SidList, SegmentList),
    SAI_ATTR_MAP(Srv6SidList, NextHopId),
};
} // namespace

namespace facebook::fboss {

WRAP_CREATE_FUNC(srv6_sidlist, SAI_OBJECT_TYPE_SRV6_SIDLIST, srv6);
WRAP_REMOVE_FUNC(srv6_sidlist, SAI_OBJECT_TYPE_SRV6_SIDLIST, srv6);
WRAP_SET_ATTR_FUNC(srv6_sidlist, SAI_OBJECT_TYPE_SRV6_SIDLIST, srv6);
WRAP_GET_ATTR_FUNC(srv6_sidlist, SAI_OBJECT_TYPE_SRV6_SIDLIST, srv6);

sai_srv6_api_t* wrappedSrv6Api() {
  static sai_srv6_api_t srv6Wrappers;

  srv6Wrappers.create_srv6_sidlist = &wrap_create_srv6_sidlist;
  srv6Wrappers.remove_srv6_sidlist = &wrap_remove_srv6_sidlist;
  srv6Wrappers.set_srv6_sidlist_attribute = &wrap_set_srv6_sidlist_attribute;
  srv6Wrappers.get_srv6_sidlist_attribute = &wrap_get_srv6_sidlist_attribute;

  return &srv6Wrappers;
}

SET_SAI_ATTRIBUTES(Srv6SidList);

} // namespace facebook::fboss
#endif
