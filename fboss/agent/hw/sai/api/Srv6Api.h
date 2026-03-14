// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/hw/sai/api/SaiApi.h"
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"
#include "fboss/agent/hw/sai/api/SaiDefaultAttributeValues.h"
#include "fboss/agent/hw/sai/api/Types.h"

#include <folly/IPAddress.h>
#include <folly/IPAddressV6.h>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
class Srv6Api;

struct SaiSrv6SidListTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_SRV6_SIDLIST;
  using SaiApiT = Srv6Api;
  struct Attributes {
    using EnumType = sai_srv6_sidlist_attr_t;
    using Type =
        SaiAttribute<EnumType, SAI_SRV6_SIDLIST_ATTR_TYPE, sai_int32_t>;
    using SegmentList = SaiAttribute<
        EnumType,
        SAI_SRV6_SIDLIST_ATTR_SEGMENT_LIST,
        std::vector<folly::IPAddressV6>>;
    using NextHopId =
        SaiAttribute<EnumType, SAI_SRV6_SIDLIST_ATTR_NEXT_HOP_ID, SaiObjectIdT>;
  };
  using AdapterKey = Srv6SidListSaiId;
  using CreateAttributes = std::tuple<
      Attributes::Type,
      std::optional<Attributes::SegmentList>,
      std::optional<Attributes::NextHopId>>;
  using AdapterHostKey = std::tuple<
      Attributes::Type,
      std::optional<Attributes::SegmentList>,
      RouterInterfaceSaiId,
      folly::IPAddress>;
};

SAI_ATTRIBUTE_NAME(Srv6SidList, Type);
SAI_ATTRIBUTE_NAME(Srv6SidList, SegmentList);
SAI_ATTRIBUTE_NAME(Srv6SidList, NextHopId);

class Srv6Api : public SaiApi<Srv6Api> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_SRV6;
  Srv6Api() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for srv6 api");
  }

 private:
  sai_status_t _create(
      Srv6SidListSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_srv6_sidlist(
        rawSaiId(id), switch_id, static_cast<uint32_t>(count), attr_list);
  }
  sai_status_t _remove(Srv6SidListSaiId key) const {
    return api_->remove_srv6_sidlist(key);
  }
  sai_status_t _getAttribute(Srv6SidListSaiId key, sai_attribute_t* attr)
      const {
    return api_->get_srv6_sidlist_attribute(key, 1, attr);
  }
  sai_status_t _setAttribute(Srv6SidListSaiId key, const sai_attribute_t* attr)
      const {
    return api_->set_srv6_sidlist_attribute(key, attr);
  }
  sai_status_t _getStats(
      Srv6SidListSaiId key,
      uint32_t num_of_counters,
      const sai_stat_id_t* counter_ids,
      sai_stats_mode_t /*mode*/,
      uint64_t* counters) const {
    return api_->get_srv6_sidlist_stats(
        key, num_of_counters, counter_ids, counters);
  }
  sai_status_t _clearStats(
      Srv6SidListSaiId key,
      uint32_t num_of_counters,
      const sai_stat_id_t* counter_ids) const {
    return api_->clear_srv6_sidlist_stats(key, num_of_counters, counter_ids);
  }

  sai_srv6_api_t* api_{nullptr};
  friend class SaiApi<Srv6Api>;
};
#endif

} // namespace facebook::fboss
