// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <cstdint>
#include <type_traits>

#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTypes.h"

namespace facebook::fboss::utility {

template <typename AddrT>
struct RouteResourceType {
  using entry = typename std::conditional_t<
      std::is_same<AddrT, folly::IPAddressV4>::value,
      RouteV4,
      RouteV6>;
  using type = typename std::unique_ptr<entry>;
};

template <typename IdT>
class ResourceCursor {
 public:
  ResourceCursor();
  IdT getNextId();
  void resetCursor(IdT current) {
    current_ = current;
  }
  IdT getCursorPosition() const {
    return current_;
  }
  IdT getId(IdT startId, uint32_t offset) const;

 private:
  IdT current_;
};

using IdV6 = typename std::pair<uint64_t, uint64_t>;
template <typename AddrT>
struct IdT {
  using type = typename std::conditional_t<
      std::is_same_v<AddrT, folly::IPAddressV4>,
      uint32_t,
      typename std::conditional_t<
          std::is_same_v<AddrT, folly::MacAddress>,
          uint64_t,
          IdV6>>;
};

template <typename ResourceT, typename IdT>
class ResourceGenerator : private ResourceCursor<IdT> {
 public:
  using BaseT = ResourceCursor<IdT>;

  virtual ResourceT get(IdT id) const = 0;
  virtual ~ResourceGenerator() {}

  /* generate an resource and update cursor */
  ResourceT getNext() {
    return get(BaseT::getNextId());
  }

  /* generate next n resources, updates cursor */
  std::vector<ResourceT> getNextN(uint32_t n) {
    std::vector<ResourceT> resources;
    for (auto i = 0; i < n; i++) {
      resources.emplace_back(getNext());
    }
    return resources;
  }

  /* generate next n resources, starting from startId, doesn't update cursor */
  std::vector<ResourceT> getN(IdT startId, uint32_t n) const {
    std::vector<ResourceT> resources;
    for (auto i = 0; i < n; i++) {
      resources.emplace_back(getId(startId, i));
    }
    return resources;
  }

  /* reset cursor */
  void startOver(IdT id) {
    BaseT::resetCursor(id);
  }

  /* return current cursor position */
  IdT getCursorPosition() {
    return BaseT::getCursorPosition();
  }
};

class MacAddressGenerator : public ResourceGenerator<
                                folly::MacAddress,
                                typename IdT<folly::MacAddress>::type> {
 public:
  using ResourceT = folly::MacAddress;
  using IdT = typename IdT<folly::MacAddress>::type;
  ResourceT get(IdT id) const override {
    return folly::MacAddress::fromHBO(id);
  }
};

template <typename AddrT>
class IPAddressGenerator
    : public ResourceGenerator<AddrT, typename IdT<AddrT>::type> {
 public:
  using ResourceT = AddrT;
  using IdT = typename IdT<AddrT>::type;

  /* simply generate an ip at cursor position id, doesn't update cursor */
  ResourceT get(IdT id) const override {
    return getIP(id);
  }

 private:
  ResourceT getIP(IdT id) const;
};

template <typename AddrT>
class PrefixGenerator : public ResourceGenerator<
                            typename facebook::fboss::RoutePrefix<AddrT>,
                            typename IdT<AddrT>::type> {
 public:
  explicit PrefixGenerator(uint8_t mask) : mask_(mask) {}
  using ResourceT = typename facebook::fboss::RoutePrefix<AddrT>;
  using IdT = typename IdT<AddrT>::type;

  /* simply generate a prefix at cursor position id, doesn't update cursor */
  ResourceT get(IdT id) const override {
    if (!mask_) {
      return ResourceT{AddrT(), mask_};
    }
    return ResourceT{ipGenerator_.get(getPrefixId(id)), mask_};
  }

 private:
  template <typename IdT>
  std::enable_if_t<std::is_same<IdT, uint32_t>::value, uint32_t> getPrefixId(
      IdT id) const {
    CHECK_LE(mask_, folly::IPAddressV4::bitCount());
    return id << (folly::IPAddressV4::bitCount() - mask_);
  }

  template <typename IdT>
  std::enable_if_t<std::is_same<IdT, IdV6>::value, IdV6> getPrefixId(
      IdT id) const {
    CHECK_LE(mask_, folly::IPAddressV6::bitCount());
    const auto kShift = folly::IPAddressV6::bitCount() - mask_;
    if (!kShift) {
      return id;
    }
    /* 128 bit id for IPv6 is represented as pair of 64 bit intgers,
      kShift >= 64,
         - In this case, shift id.second by (mask_ - 64) bits
         - id.first = id.second & id.second becomes zero
      kShift < 64,
          - both id.first and id.second will shift by kShift
          - Most (64-kShift) significant bits in id.second will be carried over
            in least significant (64-kShift) bits in id.first
    */
    if (kShift >= 64) {
      id.second <<= (kShift - 64);
      id.first = id.second;
      id.second = 0;
    } else {
      uint64_t mostSignificantBitsOfSecond = (id.second >> (64 - kShift));
      id.second <<= kShift;
      id.first <<= kShift;
      id.first |= mostSignificantBitsOfSecond;
    }
    return id;
  }

  const uint8_t mask_;
  IPAddressGenerator<AddrT> ipGenerator_;
};

template <typename AddrT>
class RouteGenerator : public ResourceGenerator<
                           typename RouteResourceType<AddrT>::type,
                           typename IdT<AddrT>::type> {
 public:
  using AdminDistance = facebook::fboss::AdminDistance;
  using InterfaceID = facebook::fboss::InterfaceID;
  using RouteT = typename RouteResourceType<AddrT>::entry;
  using ResourceT = typename RouteResourceType<AddrT>::type;
  using IdT = typename IdT<AddrT>::type;
  using Interface2Nexthops =
      std::unordered_map<InterfaceID, std::unordered_set<AddrT>>;
  using NextHopSet = RouteNextHopEntry::NextHopSet;
  RouteGenerator(
      uint8_t mask,
      const AdminDistance distance,
      const Interface2Nexthops& interface2Nexthops)
      : distance_(distance), prefixGenerator_(mask) {
    for (const auto& interfaceAndNhops : interface2Nexthops) {
      auto& nhops = interfaceAndNhops.second;
      std::for_each(
          nhops.begin(),
          nhops.end(),
          [this, &interfaceAndNhops](const auto& nhop) {
            nextHopSet_.insert(
                ResolvedNextHop(nhop, interfaceAndNhops.first, 1));
          });
    }
  }

  ResourceT get(IdT id) const override {
    const typename RouteFields<AddrT>::Prefix prefix = prefixGenerator_.get(id);
    auto route = std::make_unique<RouteT>(RouteT::makeThrift(
        prefix,
        ClientID(1), // static route
        RouteNextHopEntry(nextHopSet_, distance_)));
    route->setResolved(RouteNextHopEntry(nextHopSet_, distance_));
    return route;
  }

 private:
  AdminDistance distance_;
  NextHopSet nextHopSet_;
  PrefixGenerator<AddrT> prefixGenerator_;
};

class SakKeyHexGenerator : public ResourceGenerator<std::string, uint64_t> {
 public:
  using ResourceT = std::string;
  using IdT = uint64_t;
  ResourceT get(IdT id) const override;
};

class SakKeyIdHexGenerator : public ResourceGenerator<std::string, uint64_t> {
 public:
  using ResourceT = std::string;
  using IdT = uint64_t;
  ResourceT get(IdT id) const override;
};

} // namespace facebook::fboss::utility
