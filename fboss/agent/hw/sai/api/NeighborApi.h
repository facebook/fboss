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

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>
#include <folly/logging/xlog.h>

#include <iterator>
#include <vector>

extern "C" {
#include <sai.h>
}

namespace facebook {
namespace fboss {

struct NeighborApiParameters {
  struct Attributes {
    using EnumType = sai_neighbor_entry_attr_t;
    using DstMac = SaiAttribute<
        EnumType,
        SAI_NEIGHBOR_ENTRY_ATTR_DST_MAC_ADDRESS,
        folly::MacAddress>;
    using CreateAttributes = SaiAttributeTuple<DstMac>;
    /* implicit */ Attributes(const CreateAttributes& create) {
      std::tie(dstMac) = create.value();
    }
    CreateAttributes attrs() const {
      return {dstMac};
    }
    bool operator==(const Attributes& other) const {
      return attrs() == other.attrs();
    }
    bool operator!=(const Attributes& other) const {
      return !(*this == other);
    }
    DstMac::ValueType dstMac;
  };

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

   private:
    sai_neighbor_entry_t neighbor_entry;
  };
  using EntryType = NeighborEntry;
};

class NeighborApi : public SaiApi<NeighborApi, NeighborApiParameters> {
 public:
  NeighborApi() {
    sai_status_t status =
        sai_api_query(SAI_API_NEIGHBOR, reinterpret_cast<void**>(&api_));
    saiCheckError(status, "Failed to query for neighbor api");
  }

 private:
  sai_status_t _create(
      const NeighborApiParameters::NeighborEntry& neighborEntry,
      sai_attribute_t* attr_list,
      size_t count) {
    return api_->create_neighbor_entry(neighborEntry.entry(), count, attr_list);
  }
  sai_status_t _remove(
      const NeighborApiParameters::NeighborEntry& neighborEntry) {
    return api_->remove_neighbor_entry(neighborEntry.entry());
  }
  sai_status_t _getAttr(
      sai_attribute_t* attr,
      const NeighborApiParameters::NeighborEntry& neighborEntry) const {
    return api_->get_neighbor_entry_attribute(neighborEntry.entry(), 1, attr);
  }
  sai_status_t _setAttr(
      const sai_attribute_t* attr,
      const NeighborApiParameters::NeighborEntry& neighborEntry) {
    return api_->set_neighbor_entry_attribute(neighborEntry.entry(), attr);
  }
  sai_neighbor_api_t* api_;
  friend class SaiApi<NeighborApi, NeighborApiParameters>;
};

} // namespace fboss
} // namespace facebook

namespace std {
template <>
struct hash<facebook::fboss::NeighborApiParameters::NeighborEntry> {
  size_t operator()(
      const facebook::fboss::NeighborApiParameters::NeighborEntry& n) const;
};
} // namespace std
