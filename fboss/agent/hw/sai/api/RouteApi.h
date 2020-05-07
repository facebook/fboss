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
#include <folly/dynamic.h>
#include <folly/logging/xlog.h>

#include <iterator>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class RouteApi;

struct SaiRouteTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_ROUTE_ENTRY;
  using SaiApiT = RouteApi;
  struct Attributes {
    using EnumType = sai_route_entry_attr_t;
    using NextHopId =
        SaiAttribute<EnumType, SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID, SaiObjectIdT>;
    using PacketAction =
        SaiAttribute<EnumType, SAI_ROUTE_ENTRY_ATTR_PACKET_ACTION, sai_int32_t>;
    using Metadata = SaiAttribute<
        EnumType,
        SAI_ROUTE_ENTRY_ATTR_META_DATA,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
  };
  class RouteEntry {
   public:
    RouteEntry() {}
    RouteEntry(
        sai_object_id_t switchId,
        sai_object_id_t virtualRouterId,
        const folly::CIDRNetwork& prefix) {
      route_entry.switch_id = switchId;
      route_entry.vr_id = virtualRouterId;
      route_entry.destination = toSaiIpPrefix(prefix);
    }
    explicit RouteEntry(const sai_object_key_t& key) {
      route_entry = key.key.route_entry;
    }
    folly::CIDRNetwork destination() const {
      return fromSaiIpPrefix(route_entry.destination);
    }
    sai_object_id_t switchId() const {
      return route_entry.switch_id;
    }
    sai_object_id_t virtualRouterId() const {
      return route_entry.vr_id;
    }
    const sai_route_entry_t* entry() const {
      return &route_entry;
    }
    bool operator==(const RouteEntry& other) const {
      return (
          switchId() == other.switchId() &&
          virtualRouterId() == other.virtualRouterId() &&
          destination() == other.destination());
    }
    folly::dynamic toFollyDynamic() const;
    static RouteEntry fromFollyDynamic(const folly::dynamic& json);

    std::string toString() const;

   private:
    sai_route_entry_t route_entry{};
  };

  using AdapterKey = RouteEntry;
  using AdapterHostKey = RouteEntry;
  using CreateAttributes = std::tuple<
      Attributes::PacketAction,
      std::optional<Attributes::NextHopId>,
      std::optional<Attributes::Metadata>>;
};
template <>
struct IsSaiEntryStruct<SaiRouteTraits::RouteEntry> : public std::true_type {};

SAI_ATTRIBUTE_NAME(Route, PacketAction)
SAI_ATTRIBUTE_NAME(Route, NextHopId)
SAI_ATTRIBUTE_NAME(Route, Metadata)

class RouteApi : public SaiApi<RouteApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_ROUTE;
  RouteApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for route api");
  }

 private:
  sai_status_t _create(
      const typename SaiRouteTraits::RouteEntry& routeEntry,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_route_entry(routeEntry.entry(), count, attr_list);
  }
  sai_status_t _remove(const SaiRouteTraits::RouteEntry& routeEntry) {
    return api_->remove_route_entry(routeEntry.entry());
  }
  sai_status_t _getAttribute(
      const SaiRouteTraits::RouteEntry& routeEntry,
      sai_attribute_t* attr) const {
    return api_->get_route_entry_attribute(routeEntry.entry(), 1, attr);
  }
  sai_status_t _setAttribute(
      const SaiRouteTraits::RouteEntry& routeEntry,
      const sai_attribute_t* attr) {
    return api_->set_route_entry_attribute(routeEntry.entry(), attr);
  }

  sai_route_api_t* api_;
  friend class SaiApi<RouteApi>;
};

inline void toAppend(
    const SaiRouteTraits::RouteEntry& entry,
    std::string* result) {
  result->append(entry.toString());
}

} // namespace facebook::fboss

namespace std {
template <>
struct hash<facebook::fboss::SaiRouteTraits::RouteEntry> {
  size_t operator()(const facebook::fboss::SaiRouteTraits::RouteEntry& n) const;
};
} // namespace std
