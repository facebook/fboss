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

extern "C" {
#include <sai.h>
}

#include "fboss/agent/hw/sai/api/SaiApi.h"

#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"

namespace facebook {
namespace fboss {

struct MplsApiParameters {
  static constexpr sai_api_t ApiType = SAI_API_MPLS;

  struct Attributes {
    using EnumType = sai_inseg_entry_attr_t;
    using NextHopId =
        SaiAttribute<EnumType, SAI_INSEG_ENTRY_ATTR_NEXT_HOP_ID, SaiObjectIdT>;
    using PacketAction = SaiAttribute<
        EnumType,
        SAI_INSEG_ENTRY_ATTR_PACKET_ACTION,
        sai_packet_action_t>;
    using NumOfPop =
        SaiAttribute<EnumType, SAI_INSEG_ENTRY_ATTR_NUM_OF_POP, sai_uint8_t>;

    using CreateAttributes =
        SaiAttributeTuple<PacketAction, NumOfPop, NextHopId>;
  };

  class InSegEntry {
   public:
    InSegEntry(sai_object_id_t switchId, uint32_t label) {
      entry_.switch_id = switchId;
      entry_.label = label;
    }

    const sai_inseg_entry_t* entry() const {
      return &entry_;
    }

   private:
    sai_inseg_entry_t entry_;
  };
};

class MplsApi : public SaiApi<MplsApi, MplsApiParameters> {
 public:
  static auto constexpr ApiType = MplsApiParameters::ApiType;
  using InSegEntry = typename MplsApiParameters::InSegEntry;

  MplsApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query mpls api");
  }

  sai_status_t _create(
      const InSegEntry& labelFibEntry,
      sai_attribute_t* attr_list,
      size_t attr_count);
  sai_status_t _remove(const InSegEntry& labelFibEntry);
  sai_status_t _getAttr(sai_attribute_t* attr, const InSegEntry& entry) const;
  sai_status_t _setAttr(const sai_attribute_t* attr, const InSegEntry& entry);

 private:
  sai_mpls_api_t* api_;
  friend class SaiApi<MplsApi, MplsApiParameters>;
};

} // namespace fboss
} // namespace facebook
