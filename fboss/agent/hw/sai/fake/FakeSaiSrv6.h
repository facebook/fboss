// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <vector>
#include "fboss/agent/hw/sai/fake/FakeManager.h"

#include <folly/IPAddressV6.h>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
class FakeSaiSrv6SidList {
 public:
  FakeSaiSrv6SidList(
      sai_int32_t type,
      std::vector<folly::IPAddressV6> segmentList,
      sai_object_id_t nextHopId)
      : type(type), segmentList(std::move(segmentList)), nextHopId(nextHopId) {}
  sai_object_id_t id{0};
  sai_int32_t type;
  std::vector<folly::IPAddressV6> segmentList;
  sai_object_id_t nextHopId{SAI_NULL_OBJECT_ID};
};

using FakeSrv6SidListManager = FakeManager<sai_object_id_t, FakeSaiSrv6SidList>;

void populate_srv6_api(sai_srv6_api_t** srv6_api);
#endif

} // namespace facebook::fboss
