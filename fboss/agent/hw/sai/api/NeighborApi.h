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

#include "SaiApi.h"

#include "fboss/agent/hw/sai/api/AddressUtil.h"
#include "fboss/agent/hw/sai/api/SaiAttribute.h"

#include <folly/logging/xlog.h>
#include <folly/IPAddress.h>
#include <folly/MacAddress.h>

#include <iterator>
#include <vector>

extern "C" {
  #include <sai.h>
}

namespace facebook {
namespace fboss {

struct NeighborTypes {
  struct Attributes {
    using EnumType = sai_neighbor_entry_attr_t;
    using DstMac = SaiAttribute<
        EnumType,
        SAI_NEIGHBOR_ENTRY_ATTR_DST_MAC_ADDRESS,
        sai_mac_t,
        folly::MacAddress>;
  };

  using AttributeType = boost::variant<typename Attributes::DstMac>;

  class NeighborEntry {
   public:
    NeighborEntry(
        sai_object_id_t switchId,
        sai_object_id_t routerInterfaceId,
        const folly::IPAddress& ip) {
      neighbor_entry.switch_id = switchId;
      neighbor_entry.rif_id = routerInterfaceId;
      neighbor_entry.ip_address = toSaiIpAddress(ip);
    }
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

   private:
    sai_neighbor_entry_t neighbor_entry;
  };
  using EntryType = NeighborEntry;
  struct MemberAttributes {};
  using MemberAttributeType = boost::variant<boost::blank>;
};

class NeighborApi : public SaiApi<NeighborApi, NeighborTypes> {
 public:
  NeighborApi() {
    sai_status_t status =
        sai_api_query(SAI_API_NEIGHBOR, reinterpret_cast<void**>(&api_));
    saiCheckError(status, "Failed to query for neighbor api");
  }

 private:
  sai_status_t _create(
      const NeighborTypes::NeighborEntry& neighborEntry,
      sai_attribute_t* attr_list,
      size_t count) {
    return api_->create_neighbor_entry(neighborEntry.entry(), count, attr_list);
  }
  sai_status_t _remove(const NeighborTypes::NeighborEntry& neighborEntry) {
    return api_->remove_neighbor_entry(neighborEntry.entry());
  }
  sai_status_t _getAttr(
      sai_attribute_t* attr,
      const NeighborTypes::NeighborEntry& neighborEntry) const {
    return api_->get_neighbor_entry_attribute(neighborEntry.entry(), 1, attr);
  }
  sai_status_t _setAttr(
      const sai_attribute_t* attr,
      const NeighborTypes::NeighborEntry& neighborEntry) {
    return api_->set_neighbor_entry_attribute(neighborEntry.entry(), attr);
  }
  sai_neighbor_api_t* api_;
  friend class SaiApi<NeighborApi, NeighborTypes>;
};

} // namespace fboss
} // namespace facebook

namespace std {
template <>
struct hash<facebook::fboss::NeighborTypes::NeighborEntry> {
  size_t operator()(
      const facebook::fboss::NeighborTypes::NeighborEntry& n) const;
};
} // namespace std
