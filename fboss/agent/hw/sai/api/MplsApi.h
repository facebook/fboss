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

#include <folly/dynamic.h>

namespace facebook::fboss {

class MplsApi;

struct SaiInSegTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_INSEG_ENTRY;
  using SaiApiT = MplsApi;
  struct Attributes {
    using EnumType = sai_inseg_entry_attr_t;
    using NextHopId =
        SaiAttribute<EnumType, SAI_INSEG_ENTRY_ATTR_NEXT_HOP_ID, SaiObjectIdT>;
    using PacketAction =
        SaiAttribute<EnumType, SAI_INSEG_ENTRY_ATTR_PACKET_ACTION, sai_int32_t>;
    using NumOfPop =
        SaiAttribute<EnumType, SAI_INSEG_ENTRY_ATTR_NUM_OF_POP, sai_uint8_t>;
  };

  class InSegEntry {
   public:
    InSegEntry() {}
    InSegEntry(sai_object_id_t switchId, uint32_t label) {
      entry_.switch_id = switchId;
      entry_.label = label;
    }
    explicit InSegEntry(const sai_object_key_t& key) {
      entry_ = key.key.inseg_entry;
    }
    const sai_inseg_entry_t* entry() const {
      return &entry_;
    }
    sai_object_id_t switchId() const {
      return entry_.switch_id;
    }
    sai_object_id_t label() const {
      return entry_.label;
    }
    std::string toString() const;

    bool operator==(const InSegEntry& other) const {
      return switchId() == other.switchId() && label() == other.label();
    }
    folly::dynamic toFollyDynamic() const;
    static InSegEntry fromFollyDynamic(const folly::dynamic& json);

   private:
    sai_inseg_entry_t entry_{};
  };

  using AdapterKey = InSegEntry;
  using AdapterHostKey = InSegEntry;
  using CreateAttributes = std::tuple<
      Attributes::PacketAction,
      Attributes::NumOfPop,
      std::optional<Attributes::NextHopId>>;
};

SAI_ATTRIBUTE_NAME(InSeg, NextHopId)
SAI_ATTRIBUTE_NAME(InSeg, PacketAction)
SAI_ATTRIBUTE_NAME(InSeg, NumOfPop)

template <>
struct IsSaiEntryStruct<SaiInSegTraits::InSegEntry> : public std::true_type {};

class MplsApi : public SaiApi<MplsApi> {
 public:
  static auto constexpr ApiType = SAI_API_MPLS;

  MplsApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query mpls api");
  }

  sai_status_t _create(
      const typename SaiInSegTraits::InSegEntry& inSegEntry,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_inseg_entry(inSegEntry.entry(), count, attr_list);
  }
  sai_status_t _remove(const SaiInSegTraits::InSegEntry& inSegEntry) {
    return api_->remove_inseg_entry(inSegEntry.entry());
  }
  sai_status_t _getAttribute(
      const SaiInSegTraits::InSegEntry& inSegEntry,
      sai_attribute_t* attr) const {
    return api_->get_inseg_entry_attribute(inSegEntry.entry(), 1, attr);
  }
  sai_status_t _setAttribute(
      const SaiInSegTraits::InSegEntry& inSegEntry,
      const sai_attribute_t* attr) {
    return api_->set_inseg_entry_attribute(inSegEntry.entry(), attr);
  }

 private:
  sai_mpls_api_t* api_;
  friend class SaiApi<MplsApi>;
};

} // namespace facebook::fboss

namespace std {
template <>
struct hash<facebook::fboss::SaiInSegTraits::InSegEntry> {
  size_t operator()(
      const facebook::fboss::SaiInSegTraits::InSegEntry& entry) const;
};
} // namespace std
