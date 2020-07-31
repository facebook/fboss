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

#include "fboss/agent/hw/sai/api/AddressUtil.h"
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"
#include "fboss/agent/hw/sai/api/SaiDefaultAttributeValues.h"

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>
#include <folly/dynamic.h>
#include <folly/logging/xlog.h>

#include <iterator>
#include <tuple>
#include <vector>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class NeighborApi;

struct SaiNeighborTraits {
  static constexpr sai_object_type_t ObjectType =
      SAI_OBJECT_TYPE_NEIGHBOR_ENTRY;
  using SaiApiT = NeighborApi;
  struct Attributes {
    using EnumType = sai_neighbor_entry_attr_t;
    using DstMac = SaiAttribute<
        EnumType,
        SAI_NEIGHBOR_ENTRY_ATTR_DST_MAC_ADDRESS,
        folly::MacAddress>;
    using Metadata = SaiAttribute<
        EnumType,
        SAI_NEIGHBOR_ENTRY_ATTR_META_DATA,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
  };
  class NeighborEntry {
   public:
    NeighborEntry() {}
    NeighborEntry(
        sai_object_id_t switchId,
        sai_object_id_t routerInterfaceId,
        const folly::IPAddress& ip) {
      neighbor_entry.switch_id = switchId;
      neighbor_entry.rif_id = routerInterfaceId;
      neighbor_entry.ip_address = toSaiIpAddress(ip);
    }
    explicit NeighborEntry(const sai_object_key_t& key) {
      neighbor_entry = key.key.neighbor_entry;
    }
    // TODO MAYBE: parameterize (this? whole API?) by IP?
    folly::IPAddress ip() const {
      return fromSaiIpAddress(neighbor_entry.ip_address);
    }
    sai_object_id_t switchId() const {
      return neighbor_entry.switch_id;
    }
    sai_object_id_t routerInterfaceId() const {
      return neighbor_entry.rif_id;
    }
    const sai_neighbor_entry_t* entry() const {
      return &neighbor_entry;
    }
    bool operator==(const NeighborEntry& other) const {
      return (
          switchId() == other.switchId() &&
          routerInterfaceId() == other.routerInterfaceId() &&
          ip() == other.ip());
    }

    folly::dynamic toFollyDynamic() const;
    static NeighborEntry fromFollyDynamic(const folly::dynamic& json);
    std::string toString() const;

   private:
    sai_neighbor_entry_t neighbor_entry{};
  };
  using CreateAttributes =
      std::tuple<Attributes::DstMac, std::optional<Attributes::Metadata>>;
  using AdapterKey = NeighborEntry;
  using AdapterHostKey = NeighborEntry;
};

SAI_ATTRIBUTE_NAME(Neighbor, DstMac)
SAI_ATTRIBUTE_NAME(Neighbor, Metadata)

template <>
struct IsSaiEntryStruct<SaiNeighborTraits::NeighborEntry>
    : public std::true_type {};

class NeighborApi : public SaiApi<NeighborApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_NEIGHBOR;
  NeighborApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for neighbor api");
  }

 private:
  sai_status_t _create(
      const SaiNeighborTraits::NeighborEntry& neighborEntry,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_neighbor_entry(neighborEntry.entry(), count, attr_list);
  }
  sai_status_t _remove(const SaiNeighborTraits::NeighborEntry& neighborEntry) {
    return api_->remove_neighbor_entry(neighborEntry.entry());
  }
  sai_status_t _getAttribute(
      const SaiNeighborTraits::NeighborEntry& neighborEntry,
      sai_attribute_t* attr) const {
    return api_->get_neighbor_entry_attribute(neighborEntry.entry(), 1, attr);
  }
  sai_status_t _setAttribute(
      const SaiNeighborTraits::NeighborEntry& neighborEntry,
      const sai_attribute_t* attr) {
    return api_->set_neighbor_entry_attribute(neighborEntry.entry(), attr);
  }

  sai_neighbor_api_t* api_;
  friend class SaiApi<NeighborApi>;
};

inline void toAppend(
    const SaiNeighborTraits::NeighborEntry& entry,
    std::string* result) {
  result->append(entry.toString());
}

} // namespace facebook::fboss

namespace std {
template <>
struct hash<facebook::fboss::SaiNeighborTraits::NeighborEntry> {
  size_t operator()(
      const facebook::fboss::SaiNeighborTraits::NeighborEntry& n) const;
};
} // namespace std
