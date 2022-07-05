// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/hw/sai/api/SaiApi.h"
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"
#include "fboss/agent/hw/sai/api/SaiDefaultAttributeValues.h"
#include "fboss/agent/hw/sai/api/Types.h"

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class TunnelApi;

struct SaiTunnelTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_TUNNEL;
  using SaiApiT = TunnelApi;
  struct Attributes {
    using EnumType = sai_tunnel_attr_t;
  };
};

struct SaiTunnelTermTraits {
  static constexpr sai_object_type_t ObjectType =
      SAI_OBJECT_TYPE_TUNNEL_TERM_TABLE_ENTRY;
  using SaiApiT = TunnelApi;
  struct Attributes {
    using EnumType = sai_tunnel_term_table_entry_attr_t;
  };
};

class TunnelApi : public SaiApi<TunnelApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_TUNNEL;
  TunnelApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for tunnel api");
  }

 private:
  sai_tunnel_api_t* api_;
  friend class SaiApi<TunnelApi>;
};

} // namespace facebook::fboss
