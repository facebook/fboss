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
// NOLINTNEXTLINE(facebook-avoid-non-const-global-variables)
std::map<int32_t, std::pair<std::string, std::size_t>> _MySidEntryMap{
    SAI_ATTR_MAP(MySidEntry, EndpointBehavior),
    SAI_ATTR_MAP(MySidEntry, EndpointBehaviorFlavor),
    SAI_ATTR_MAP(MySidEntry, NextHopId),
    SAI_ATTR_MAP(MySidEntry, Vrf),
};
} // namespace

namespace facebook::fboss {

WRAP_CREATE_FUNC(srv6_sidlist, SAI_OBJECT_TYPE_SRV6_SIDLIST, srv6);
WRAP_REMOVE_FUNC(srv6_sidlist, SAI_OBJECT_TYPE_SRV6_SIDLIST, srv6);
WRAP_SET_ATTR_FUNC(srv6_sidlist, SAI_OBJECT_TYPE_SRV6_SIDLIST, srv6);
WRAP_GET_ATTR_FUNC(srv6_sidlist, SAI_OBJECT_TYPE_SRV6_SIDLIST, srv6);

sai_status_t wrap_create_my_sid_entry(
    const sai_my_sid_entry_t* my_sid_entry,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->srv6Api_->create_my_sid_entry(
      my_sid_entry, attr_count, attr_list);

  SaiTracer::getInstance()->logMySidEntryCreateFn(
      my_sid_entry, attr_count, attr_list, rv);
  return rv;
}

sai_status_t wrap_remove_my_sid_entry(const sai_my_sid_entry_t* my_sid_entry) {
  auto rv =
      SaiTracer::getInstance()->srv6Api_->remove_my_sid_entry(my_sid_entry);

  SaiTracer::getInstance()->logMySidEntryRemoveFn(my_sid_entry, rv);
  return rv;
}

sai_status_t wrap_set_my_sid_entry_attribute(
    const sai_my_sid_entry_t* my_sid_entry,
    const sai_attribute_t* attr) {
  auto rv = SaiTracer::getInstance()->srv6Api_->set_my_sid_entry_attribute(
      my_sid_entry, attr);

  SaiTracer::getInstance()->logMySidEntrySetAttrFn(my_sid_entry, attr, rv);
  return rv;
}

sai_status_t wrap_get_my_sid_entry_attribute(
    const sai_my_sid_entry_t* my_sid_entry,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->srv6Api_->get_my_sid_entry_attribute(
      my_sid_entry, attr_count, attr_list);
}

sai_srv6_api_t* wrappedSrv6Api() {
  static sai_srv6_api_t srv6Wrappers;

  srv6Wrappers.create_srv6_sidlist = &wrap_create_srv6_sidlist;
  srv6Wrappers.remove_srv6_sidlist = &wrap_remove_srv6_sidlist;
  srv6Wrappers.set_srv6_sidlist_attribute = &wrap_set_srv6_sidlist_attribute;
  srv6Wrappers.get_srv6_sidlist_attribute = &wrap_get_srv6_sidlist_attribute;

  srv6Wrappers.create_my_sid_entry = &wrap_create_my_sid_entry;
  srv6Wrappers.remove_my_sid_entry = &wrap_remove_my_sid_entry;
  srv6Wrappers.set_my_sid_entry_attribute = &wrap_set_my_sid_entry_attribute;
  srv6Wrappers.get_my_sid_entry_attribute = &wrap_get_my_sid_entry_attribute;

  return &srv6Wrappers;
}

SET_SAI_ATTRIBUTES(Srv6SidList);
SET_SAI_ATTRIBUTES(MySidEntry);

} // namespace facebook::fboss
#endif
