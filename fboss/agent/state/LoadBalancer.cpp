/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/LoadBalancer.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/NodeBase-defs.h"

#include <folly/MacAddress.h>
#include <folly/hash/Hash.h>

#include <algorithm>
#include <limits>
#include <type_traits>

namespace {
static constexpr folly::StringPiece kLoadBalancerID{"id"};
static constexpr folly::StringPiece kAlgorithm{"algorithm"};
static constexpr folly::StringPiece kSeed{"seed"};
static constexpr folly::StringPiece kIPv4Fields{"v4Fields"};
static constexpr folly::StringPiece kIPv6Fields{"v6Fields"};
static constexpr folly::StringPiece kTransportFields{"transportFields"};
static constexpr folly::StringPiece kMPLSFields{"mplsFields"};
static constexpr folly::StringPiece kUdfGroupIds{"udfGroupIds"};
}; // namespace

namespace facebook::fboss {

LoadBalancer::LoadBalancer(
    LoadBalancerID id,
    cfg::HashingAlgorithm algorithm,
    uint32_t seed,
    LoadBalancer::IPv4Fields v4Fields,
    LoadBalancer::IPv6Fields v6Fields,
    LoadBalancer::TransportFields transportFields,
    LoadBalancer::MPLSFields mplsFields,
    LoadBalancerFields::UdfGroupIds udfGroupIds)
    : ThriftStructNode<LoadBalancer, state::LoadBalancerFields>() {
  set<switch_state_tags::id>(id);
  set<switch_state_tags::algorithm>(algorithm);
  set<switch_state_tags::seed>(seed);
  for (auto field : v4Fields) {
    get<switch_state_tags::v4Fields>()->emplace(field);
  }
  for (auto field : v6Fields) {
    get<switch_state_tags::v6Fields>()->emplace(field);
  }
  for (auto field : transportFields) {
    get<switch_state_tags::transportFields>()->emplace(field);
  }
  for (auto field : mplsFields) {
    get<switch_state_tags::mplsFields>()->emplace(field);
  }
  for (auto udfGroupId : udfGroupIds) {
    get<switch_state_tags::udfGroups>()->emplace_back(udfGroupId);
  }
}

LoadBalancerID LoadBalancer::getID() const {
  return get<switch_state_tags::id>()->cref();
}

uint32_t LoadBalancer::getSeed() const {
  return get<switch_state_tags::seed>()->cref();
}

cfg::HashingAlgorithm LoadBalancer::getAlgorithm() const {
  return get<switch_state_tags::algorithm>()->cref();
}

LoadBalancer::IPv4FieldsRange LoadBalancer::getIPv4Fields() const {
  if (auto fields = get<switch_state_tags::v4Fields>()) {
    return folly::range(fields->cbegin(), fields->cend());
  }
  return LoadBalancer::IPv4FieldsRange{};
}

LoadBalancer::IPv6FieldsRange LoadBalancer::getIPv6Fields() const {
  if (auto fields = get<switch_state_tags::v6Fields>()) {
    return folly::range(fields->cbegin(), fields->cend());
  }
  return LoadBalancer::IPv6FieldsRange{};
}

LoadBalancer::TransportFieldsRange LoadBalancer::getTransportFields() const {
  if (auto fields = get<switch_state_tags::transportFields>()) {
    return folly::range(fields->cbegin(), fields->cend());
  }
  return LoadBalancer::TransportFieldsRange{};
}

LoadBalancer::UdfGroupIds LoadBalancer::getUdfGroupIds() const {
  return get<switch_state_tags::udfGroups>()->toThrift();
}

LoadBalancer::MPLSFieldsRange LoadBalancer::getMPLSFields() const {
  if (auto fields = get<switch_state_tags::mplsFields>()) {
    return folly::range(fields->cbegin(), fields->cend());
  }
  return LoadBalancer::MPLSFieldsRange{};
}

std::shared_ptr<LoadBalancer> LoadBalancer::fromThrift(
    const state::LoadBalancerFields& fields) {
  return std::make_shared<LoadBalancer>(fields);
}

template class ThriftStructNode<LoadBalancer, state::LoadBalancerFields>;

} // namespace facebook::fboss
