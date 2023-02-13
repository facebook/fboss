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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

#include "folly/MacAddress.h"
#include "folly/Range.h"
#include "folly/dynamic.h"

#include "boost/container/flat_set.hpp"

namespace facebook::fboss {

struct LoadBalancerFields
    : public ThriftyFields<LoadBalancerFields, state::LoadBalancerFields> {
  using IPv4Field = cfg::IPv4Field;
  using IPv4Fields = boost::container::flat_set<IPv4Field>;

  using IPv6Field = cfg::IPv6Field;
  using IPv6Fields = boost::container::flat_set<IPv6Field>;

  using TransportField = cfg::TransportField;
  using TransportFields = boost::container::flat_set<TransportField>;

  using MPLSField = cfg::MPLSField;
  using MPLSFields = boost::container::flat_set<MPLSField>;

  using UdfGroupIds = std::vector<std::string>;

  state::LoadBalancerFields toThrift() const override;
  static LoadBalancerFields fromThrift(state::LoadBalancerFields const& fields);

  LoadBalancerFields(
      LoadBalancerID id,
      cfg::HashingAlgorithm algorithm,
      uint32_t seed,
      IPv4Fields v4Fields,
      IPv6Fields v6Fields,
      TransportFields transportFields,
      MPLSFields mplsFields = MPLSFields{},
      UdfGroupIds udfGroupIds = UdfGroupIds{});

  template <typename Fn>
  void forEachChild(Fn /* unused */) {}

  const LoadBalancerID id_;
  cfg::HashingAlgorithm algorithm_;
  uint32_t seed_;
  IPv4Fields v4Fields_;
  IPv6Fields v6Fields_;
  TransportFields transportFields_;
  MPLSFields mplsFields_;
  UdfGroupIds udfGroupIds_;
};

USE_THRIFT_COW(LoadBalancer);

/* A LoadBalancer represents a logical point in the data-path which may
 * distribute traffic between any number of load-bearing elements.
 *
 * There are currently two such supported load-balancing points:
 *
 * 1) AggregatePort load-balancer, which distributes frames across physical
 *    ports belonging to a given aggregate (aka port-channel).
 * 2) ECMP load-balancer, which distributes IP packets across multiple next hops
 *
 * Other examples of load-balancing performed in commodity switching ASICs (but
 * which we don't support) include link aggregation failover.
 *
 * The design of this SwitchState node departs from that of others: while other
 * SwitchState nodes are identified by opaque integer values, LoadBalancer
 * identifiers are taken from the enum LoadBalancerID. Because the enum is a
 * small, fixed domain, LoadBalancerIDs have semantics. In contrast, other
 * SwitchState node IDs, such as InterfaceID and AggregatePortID, are arbitrary
 * and thus devoid of semantics.
 *
 * This corresponds to the following observation: a switching ASIC's points of
 * load-balancing are fixed and so it does not follow to support abitrary
 * handles on LoadBalancer SwitchState nodes.
 */

class LoadBalancer
    : public ThriftStructNode<LoadBalancer, state::LoadBalancerFields> {
 public:
  template <typename T>
  struct Fields {
    using ElementType = T;
    using ElemnentTC = apache::thrift::type_class::enumeration;
    using FieldsTC = apache::thrift::type_class::set<ElemnentTC>;
    using FieldsType =
        thrift_cow::ThriftSetNode<FieldsTC, std::set<ElementType>>;
    using const_iterator = typename FieldsType::Fields::const_iterator;
  };

  template <typename T>
  using FieldsConstIter = typename Fields<T>::const_iterator;

  using BaseT = ThriftStructNode<LoadBalancer, state::LoadBalancerFields>;
  using IPv4Field = LoadBalancerFields::IPv4Field;
  using IPv4Fields = LoadBalancerFields::IPv4Fields;
  using IPv4FieldsRange = folly::Range<FieldsConstIter<cfg::IPv4Field>>;

  using IPv6Field = LoadBalancerFields::IPv6Field;
  using IPv6Fields = LoadBalancerFields::IPv6Fields;
  using IPv6FieldsRange = folly::Range<FieldsConstIter<cfg::IPv6Field>>;

  using TransportField = LoadBalancerFields::TransportField;
  using TransportFields = LoadBalancerFields::TransportFields;
  using TransportFieldsRange =
      folly::Range<FieldsConstIter<cfg::TransportField>>;

  using MPLSField = LoadBalancerFields::MPLSField;
  using MPLSFields = LoadBalancerFields::MPLSFields;
  using UdfGroupIds = LoadBalancerFields::UdfGroupIds;
  using MPLSFieldsRange = folly::Range<FieldsConstIter<cfg::MPLSField>>;

  LoadBalancer(
      LoadBalancerID id,
      cfg::HashingAlgorithm algorithm,
      uint32_t seed,
      IPv4Fields v4Fields,
      IPv6Fields v6Fields,
      TransportFields transportFields,
      MPLSFields mplsFields,
      UdfGroupIds udfGroupIds);

  LoadBalancerID getID() const;
  uint32_t getSeed() const;
  cfg::HashingAlgorithm getAlgorithm() const;
  IPv4FieldsRange getIPv4Fields() const;
  IPv6FieldsRange getIPv6Fields() const;
  TransportFieldsRange getTransportFields() const;
  MPLSFieldsRange getMPLSFields() const;
  UdfGroupIds getUdfGroupIds() const;

  static std::shared_ptr<LoadBalancer> fromFollyDynamicLegacy(
      const folly::dynamic& json);
  static std::shared_ptr<LoadBalancer> fromFollyDynamic(
      const folly::dynamic& json) {
    return fromFollyDynamicLegacy(json);
  }
  folly::dynamic toFollyDynamicLegacy() const;
  folly::dynamic toFollyDynamic() const override {
    return toFollyDynamicLegacy();
  }

  static std::shared_ptr<LoadBalancer> fromThrift(
      const state::LoadBalancerFields& fields);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
