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
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/types.h"

#include "folly/Range.h"
#include "folly/dynamic.h"

#include "boost/container/flat_set.hpp"

namespace facebook::fboss {

class Platform;

struct LoadBalancerFields {
  using IPv4Field = cfg::IPv4Field;
  using IPv4Fields = boost::container::flat_set<IPv4Field>;

  using IPv6Field = cfg::IPv6Field;
  using IPv6Fields = boost::container::flat_set<IPv6Field>;

  using TransportField = cfg::TransportField;
  using TransportFields = boost::container::flat_set<TransportField>;

  using MPLSField = cfg::MPLSField;
  using MPLSFields = boost::container::flat_set<MPLSField>;

  LoadBalancerFields(
      LoadBalancerID id,
      cfg::HashingAlgorithm algorithm,
      uint32_t seed,
      IPv4Fields v4Fields,
      IPv6Fields v6Fields,
      TransportFields transportFields,
      MPLSFields mplsFields = MPLSFields{});

  template <typename Fn>
  void forEachChild(Fn /* unused */) {}

  const LoadBalancerID id_;
  cfg::HashingAlgorithm algorithm_;
  int32_t seed_;
  IPv4Fields v4Fields_;
  IPv6Fields v6Fields_;
  TransportFields transportFields_;
  MPLSFields mplsFields_;
};

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
class LoadBalancer : public NodeBaseT<LoadBalancer, LoadBalancerFields> {
 public:
  using IPv4Field = LoadBalancerFields::IPv4Field;
  using IPv4Fields = LoadBalancerFields::IPv4Fields;
  using IPv4FieldsRange =
      folly::Range<LoadBalancerFields::IPv4Fields::const_iterator>;

  using IPv6Field = LoadBalancerFields::IPv6Field;
  using IPv6Fields = LoadBalancerFields::IPv6Fields;
  using IPv6FieldsRange =
      folly::Range<LoadBalancerFields::IPv6Fields::const_iterator>;

  using TransportField = LoadBalancerFields::TransportField;
  using TransportFields = LoadBalancerFields::TransportFields;
  using TransportFieldsRange =
      folly::Range<LoadBalancerFields::TransportFields::const_iterator>;

  using MPLSField = LoadBalancerFields::MPLSField;
  using MPLSFields = LoadBalancerFields::MPLSFields;
  using MPLSFieldsRange =
      folly::Range<LoadBalancerFields::MPLSFields::const_iterator>;

  LoadBalancer(
      LoadBalancerID id,
      cfg::HashingAlgorithm algorithm,
      uint32_t seed,
      IPv4Fields v4Fields,
      IPv6Fields v6Fields,
      TransportFields transportFields,
      MPLSFields mplsFields);

  LoadBalancerID getID() const;
  uint32_t getSeed() const;
  cfg::HashingAlgorithm getAlgorithm() const;
  IPv4FieldsRange getIPv4Fields() const;
  IPv6FieldsRange getIPv6Fields() const;
  TransportFieldsRange getTransportFields() const;
  MPLSFieldsRange getMPLSFields() const;

  static std::shared_ptr<LoadBalancer> fromFollyDynamic(
      const folly::dynamic& json);
  folly::dynamic toFollyDynamic() const override;

  bool operator==(const LoadBalancer& rhs) const;
  bool operator!=(const LoadBalancer& rhs) const;

  static uint32_t generateDeterministicSeed(
      LoadBalancerID loadBalancerID,
      const Platform* platform);

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
