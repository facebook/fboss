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

#include <iterator>

extern "C" {
  #include <sai.h>
}

namespace facebook {
namespace fboss {

struct RouteTypes {
  struct Attributes {
    using EnumType = sai_route_entry_attr_t;
    using NextHopId = SaiAttribute<
        EnumType,
        SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID,
        sai_object_id_t,
        SaiObjectIdT>;
  };
  using AttributeType = boost::variant<Attributes::NextHopId>;

  class RouteEntry {
   public:
    RouteEntry(
        sai_object_id_t switchId,
        sai_object_id_t virtualRouterId,
        const folly::CIDRNetwork& prefix) {
      route_entry.switch_id = switchId;
      route_entry.vr_id = virtualRouterId;
      route_entry.destination = toSaiIpPrefix(prefix);
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

   private:
    sai_route_entry_t route_entry;
  };
  using EntryType = RouteEntry;
  struct MemberAttributes {};
  using MemberAttributeType = boost::variant<boost::blank>;
};

class RouteApi : public SaiApi<RouteApi, RouteTypes> {
 public:
    RouteApi() {
      sai_status_t status =
          sai_api_query(SAI_API_ROUTE, reinterpret_cast<void**>(&api_));
      saiCheckError(status, "Failed to query for route api");
  }
 private:
  sai_status_t _create(
      const RouteTypes::RouteEntry& routeEntry,
      sai_attribute_t* attr_list,
      size_t count) {
    return api_->create_route_entry(routeEntry.entry(), count, attr_list);
  }
  sai_status_t _remove(const RouteTypes::RouteEntry& routeEntry) {
    return api_->remove_route_entry(routeEntry.entry());
  }
  sai_status_t _getAttr(
      sai_attribute_t* attr,
      const RouteTypes::RouteEntry& routeEntry) const {
    return api_->get_route_entry_attribute(routeEntry.entry(), 1, attr);
  }
  sai_status_t _setAttr(
      const sai_attribute_t* attr,
      const RouteTypes::RouteEntry& routeEntry) {
    return api_->set_route_entry_attribute(routeEntry.entry(), attr);
  }
  sai_route_api_t* api_;
  friend class SaiApi<RouteApi, RouteTypes>;
};

} // namespace fboss
} // namespace facebook

namespace std {
template <>
struct hash<facebook::fboss::RouteTypes::RouteEntry> {
  size_t operator()(const facebook::fboss::RouteTypes::RouteEntry& n) const;
};
} // namespace std
