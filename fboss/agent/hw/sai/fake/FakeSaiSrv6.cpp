// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/fake/FakeSaiSrv6.h"
#include "fboss/agent/hw/sai/api/AddressUtil.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
namespace {

using facebook::fboss::FakeSai;

sai_status_t create_srv6_sidlist_fn(
    sai_object_id_t* srv6_sidlist_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  sai_int32_t type{};
  std::vector<folly::IPAddressV6> segmentList;
  sai_object_id_t nextHopId{SAI_NULL_OBJECT_ID};
  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_SRV6_SIDLIST_ATTR_TYPE:
        type = attr_list[i].value.s32;
        break;
      case SAI_SRV6_SIDLIST_ATTR_SEGMENT_LIST: {
        const auto& list = attr_list[i].value.segmentlist;
        segmentList.reserve(list.count);
        for (uint32_t j = 0; j < list.count; j++) {
          segmentList.push_back(
              facebook::fboss::fromSaiIpAddress(list.list[j]));
        }
        break;
      }
      case SAI_SRV6_SIDLIST_ATTR_NEXT_HOP_ID:
        nextHopId = attr_list[i].value.oid;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  *srv6_sidlist_id =
      fs->srv6SidListManager.create(type, std::move(segmentList), nextHopId);
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_srv6_sidlist_fn(sai_object_id_t srv6_sidlist_id) {
  auto fs = FakeSai::getInstance();
  fs->srv6SidListManager.remove(srv6_sidlist_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_srv6_sidlist_attribute_fn(
    sai_object_id_t srv6_sidlist_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& sidList = fs->srv6SidListManager.get(srv6_sidlist_id);
  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr[i].id) {
      case SAI_SRV6_SIDLIST_ATTR_TYPE:
        attr[i].value.s32 = sidList.type;
        break;
      case SAI_SRV6_SIDLIST_ATTR_SEGMENT_LIST: {
        if (attr[i].value.segmentlist.count <
            static_cast<uint32_t>(sidList.segmentList.size())) {
          attr[i].value.segmentlist.count =
              static_cast<uint32_t>(sidList.segmentList.size());
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        attr[i].value.segmentlist.count =
            static_cast<uint32_t>(sidList.segmentList.size());
        for (size_t j = 0; j < sidList.segmentList.size(); j++) {
          facebook::fboss::toSaiIpAddressV6(
              sidList.segmentList[j], &attr[i].value.segmentlist.list[j]);
        }
        break;
      }
      case SAI_SRV6_SIDLIST_ATTR_NEXT_HOP_ID:
        attr[i].value.oid = sidList.nextHopId;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_srv6_sidlist_attribute_fn(
    sai_object_id_t srv6_sidlist_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& sidList = fs->srv6SidListManager.get(srv6_sidlist_id);
  switch (attr->id) {
    case SAI_SRV6_SIDLIST_ATTR_TYPE:
      sidList.type = attr->value.s32;
      break;
    case SAI_SRV6_SIDLIST_ATTR_SEGMENT_LIST: {
      const auto& list = attr->value.segmentlist;
      sidList.segmentList.clear();
      sidList.segmentList.reserve(list.count);
      for (uint32_t j = 0; j < list.count; j++) {
        sidList.segmentList.push_back(
            facebook::fboss::fromSaiIpAddress(list.list[j]));
      }
      break;
    }
    case SAI_SRV6_SIDLIST_ATTR_NEXT_HOP_ID:
      sidList.nextHopId = attr->value.oid;
      break;
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_srv6_sidlist_stats_fn(
    sai_object_id_t /*srv6_sidlist*/,
    uint32_t num_of_counters,
    const sai_stat_id_t* /*counter_ids*/,
    uint64_t* counters) {
  for (uint32_t i = 0; i < num_of_counters; ++i) {
    counters[i] = 0;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_srv6_sidlist_stats_ext_fn(
    sai_object_id_t srv6_sidlist,
    uint32_t num_of_counters,
    const sai_stat_id_t* counter_ids,
    sai_stats_mode_t /*mode*/,
    uint64_t* counters) {
  return get_srv6_sidlist_stats_fn(
      srv6_sidlist, num_of_counters, counter_ids, counters);
}

sai_status_t clear_srv6_sidlist_stats_fn(
    sai_object_id_t /*srv6_sidlist*/,
    uint32_t /*number_of_counters*/,
    const sai_stat_id_t* /*counter_ids*/) {
  return SAI_STATUS_SUCCESS;
}

} // namespace

namespace facebook::fboss {
// NOLINTNEXTLINE(facebook-avoid-non-const-global-variables)
static sai_srv6_api_t _srv6_api;

void populate_srv6_api(sai_srv6_api_t** srv6_api) {
  _srv6_api.create_srv6_sidlist = &create_srv6_sidlist_fn;
  _srv6_api.remove_srv6_sidlist = &remove_srv6_sidlist_fn;
  _srv6_api.get_srv6_sidlist_attribute = &get_srv6_sidlist_attribute_fn;
  _srv6_api.set_srv6_sidlist_attribute = &set_srv6_sidlist_attribute_fn;

  _srv6_api.get_srv6_sidlist_stats = &get_srv6_sidlist_stats_fn;
  _srv6_api.get_srv6_sidlist_stats_ext = &get_srv6_sidlist_stats_ext_fn;
  _srv6_api.clear_srv6_sidlist_stats = &clear_srv6_sidlist_stats_fn;

  *srv6_api = &_srv6_api;
}

} // namespace facebook::fboss
#endif
