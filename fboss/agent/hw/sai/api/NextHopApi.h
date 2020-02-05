/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/hw/sai/api/SaiApi.h"
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"
#include "fboss/agent/hw/sai/api/Types.h"

#include <folly/logging/xlog.h>

#include <tuple>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class NextHopApi;

struct SaiNextHopTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_NEXT_HOP;
  using SaiApiT = NextHopApi;
  struct Attributes {
    using EnumType = sai_next_hop_attr_t;
    using Ip = SaiAttribute<EnumType, SAI_NEXT_HOP_ATTR_IP, folly::IPAddress>;
    using RouterInterfaceId = SaiAttribute<
        EnumType,
        SAI_NEXT_HOP_ATTR_ROUTER_INTERFACE_ID,
        SaiObjectIdT>;
    using LabelStack = SaiAttribute<
        EnumType,
        SAI_NEXT_HOP_ATTR_LABELSTACK,
        std::vector<sai_uint32_t>>;
    using Type = SaiAttribute<EnumType, SAI_NEXT_HOP_ATTR_TYPE, sai_int32_t>;
  };

  using AdapterKey = NextHopSaiId;
  using AdapterHostKey = std::tuple<
      Attributes::RouterInterfaceId,
      Attributes::Ip,
      std::optional<Attributes::LabelStack>>;
  using CreateAttributes = std::tuple<
      Attributes::Type,
      Attributes::RouterInterfaceId,
      Attributes::Ip,
      std::optional<Attributes::LabelStack>>;
};

class NextHopApi : public SaiApi<NextHopApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_NEXT_HOP;
  NextHopApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for next hop api");
  }

 private:
  sai_status_t _create(
      NextHopSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_next_hop(rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _remove(NextHopSaiId next_hop_id) {
    return api_->remove_next_hop(next_hop_id);
  }
  sai_status_t _getAttribute(NextHopSaiId id, sai_attribute_t* attr) const {
    return api_->get_next_hop_attribute(id, 1, attr);
  }
  sai_status_t _setAttribute(NextHopSaiId id, const sai_attribute_t* attr) {
    return api_->set_next_hop_attribute(id, attr);
  }

  sai_next_hop_api_t* api_;
  friend class SaiApi<NextHopApi>;
};

} // namespace facebook::fboss
