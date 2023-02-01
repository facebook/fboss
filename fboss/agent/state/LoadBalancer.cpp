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

LoadBalancerFields::LoadBalancerFields(
    LoadBalancerID id,
    cfg::HashingAlgorithm algorithm,
    uint32_t seed,
    LoadBalancerFields::IPv4Fields v4Fields,
    LoadBalancerFields::IPv6Fields v6Fields,
    LoadBalancerFields::TransportFields transportFields,
    LoadBalancerFields::MPLSFields mplsFields,
    LoadBalancerFields::UdfGroupIds udfGroupIds)
    : id_(id),
      algorithm_(algorithm),
      seed_(seed),
      v4Fields_(v4Fields),
      v6Fields_(v6Fields),
      transportFields_(transportFields),
      mplsFields_(mplsFields),
      udfGroupIds_(udfGroupIds) {}

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

std::shared_ptr<LoadBalancer> LoadBalancer::fromFollyDynamicLegacy(
    const folly::dynamic& json) {
  auto id = static_cast<LoadBalancerID>(json[kLoadBalancerID].asInt());
  switch (id) {
    case LoadBalancerID::AGGREGATE_PORT:
    case LoadBalancerID::ECMP:
      // valid
      break;
    default:
      throw FbossError("Invalid LoadBalancerID ", id);
  }

  auto algorithm = static_cast<cfg::HashingAlgorithm>(json[kAlgorithm].asInt());
  switch (algorithm) {
    case cfg::HashingAlgorithm::CRC16_CCITT:
    case cfg::HashingAlgorithm::CRC32_LO:
    case cfg::HashingAlgorithm::CRC32_HI:
    case cfg::HashingAlgorithm::CRC32_ETHERNET_LO:
    case cfg::HashingAlgorithm::CRC32_ETHERNET_HI:
    case cfg::HashingAlgorithm::CRC32_KOOPMAN_LO:
    case cfg::HashingAlgorithm::CRC32_KOOPMAN_HI:
      // valid
      break;
    default:
      throw FbossError("Invalid Algorithm ", algorithm);
  }

  auto seed = json[kSeed].asInt();
  if (seed < 0 || seed > std::numeric_limits<uint32_t>::max()) {
    throw FbossError("Invalid seed ", seed);
  }

  auto serializedIPv4Fields = json[kIPv4Fields];
  IPv4Fields ipv4Fields;
  int64_t rawIPv4Field;
  for (const auto& serializedIPv4Field : serializedIPv4Fields) {
    rawIPv4Field = serializedIPv4Field.asInt();
    auto positionAndSuccess =
        ipv4Fields.insert(static_cast<IPv4Field>(rawIPv4Field));
    if (!positionAndSuccess.second) {
      throw FbossError("Duplicate IPv4 serialized field ", rawIPv4Field);
    }
  }

  auto serializedIPv6Fields = json[kIPv6Fields];
  IPv6Fields ipv6Fields;
  int64_t rawIPv6Field;
  for (const auto& serializedIPv6Field : serializedIPv6Fields) {
    rawIPv6Field = serializedIPv6Field.asInt();
    auto positionAndSuccess =
        ipv6Fields.insert(static_cast<IPv6Field>(rawIPv6Field));
    if (!positionAndSuccess.second) {
      throw FbossError("Duplicate IPv6 serialized field ", rawIPv6Field);
    }
  }

  auto serializedTransportFields = json[kTransportFields];
  TransportFields transportFields;
  int64_t rawTransportField;
  for (const auto& serializedTransportField : serializedTransportFields) {
    rawTransportField = serializedTransportField.asInt();
    auto positionAndSuccess =
        transportFields.insert(static_cast<TransportField>(rawTransportField));
    if (!positionAndSuccess.second) {
      throw FbossError(
          "Duplicate Transport serialized field ", rawTransportField);
    }
  }

  const auto& serializedMPLSFields =
      json.find(kMPLSFields) != json.items().end() ? json[kMPLSFields]
                                                   : folly::dynamic::array();
  MPLSFields mplsFields;
  for (const auto& serializedMPLSField : serializedMPLSFields) {
    auto rawMPLSField = serializedMPLSField.asInt();
    auto positionAndSuccess =
        mplsFields.insert(static_cast<MPLSField>(rawMPLSField));
    if (!positionAndSuccess.second) {
      throw FbossError("Duplicate MPLS serialized field ", rawMPLSField);
    }
  }
  auto fields = LoadBalancerFields(
      id,
      algorithm,
      seed,
      std::move(ipv4Fields),
      std::move(ipv6Fields),
      std::move(transportFields),
      mplsFields);
  return std::make_shared<LoadBalancer>(fields.toThrift());
}

folly::dynamic LoadBalancer::toFollyDynamicLegacy() const {
  folly::dynamic serialized = folly::dynamic::object();

  using IDType = std::underlying_type<LoadBalancerID>::type;
  serialized[kLoadBalancerID] = static_cast<IDType>(getID());

  using AlgorithmType = std::underlying_type<cfg::HashingAlgorithm>::type;
  serialized[kAlgorithm] = static_cast<AlgorithmType>(getAlgorithm());

  serialized[kSeed] = getSeed();

  serialized[kIPv4Fields] = folly::dynamic::array();
  for (const auto& ipv4Field : getIPv4Fields()) {
    serialized[kIPv4Fields].push_back(static_cast<int64_t>(ipv4Field->cref()));
  }

  serialized[kIPv6Fields] = folly::dynamic::array();
  for (const auto& ipv6Field : getIPv6Fields()) {
    serialized[kIPv6Fields].push_back(static_cast<int64_t>(ipv6Field->cref()));
  }

  serialized[kTransportFields] = folly::dynamic::array();
  for (const auto& transportField : getTransportFields()) {
    serialized[kTransportFields].push_back(
        static_cast<int64_t>(transportField->cref()));
  }

  serialized[kMPLSFields] = folly::dynamic::array();
  for (const auto& mplsField : getMPLSFields()) {
    serialized[kMPLSFields].push_back(static_cast<uint8_t>(mplsField->cref()));
  }
  return serialized;
}

state::LoadBalancerFields LoadBalancerFields::toThrift() const {
  state::LoadBalancerFields thriftLoadBalancerFields{};
  thriftLoadBalancerFields.id() = id_;
  thriftLoadBalancerFields.algorithm() = algorithm_;
  thriftLoadBalancerFields.seed() = seed_;
  thriftLoadBalancerFields.transportFields() = std::set<cfg::TransportField>();
  for (auto field : transportFields_) {
    thriftLoadBalancerFields.transportFields()->insert(field);
  }
  thriftLoadBalancerFields.v4Fields() = std::set<cfg::IPv4Field>();
  for (auto field : v4Fields_) {
    thriftLoadBalancerFields.v4Fields()->insert(field);
  }
  thriftLoadBalancerFields.v6Fields() = std::set<cfg::IPv6Field>();
  for (auto field : v6Fields_) {
    thriftLoadBalancerFields.v6Fields()->insert(field);
  }
  thriftLoadBalancerFields.mplsFields() = std::set<cfg::MPLSField>();
  for (auto field : mplsFields_) {
    thriftLoadBalancerFields.mplsFields()->insert(field);
  }

  return thriftLoadBalancerFields;
}

LoadBalancerFields LoadBalancerFields::fromThrift(
    state::LoadBalancerFields const& thriftLoadBalancerFields) {
  LoadBalancerFields::IPv4Fields v4;
  LoadBalancerFields::IPv6Fields v6;
  LoadBalancerFields::TransportFields tr;
  LoadBalancerFields::MPLSFields mpls;

  for (auto& field : *thriftLoadBalancerFields.v4Fields()) {
    v4.insert(field);
  }
  for (auto& field : *thriftLoadBalancerFields.v6Fields()) {
    v6.insert(field);
  }
  for (auto& field : *thriftLoadBalancerFields.transportFields()) {
    tr.insert(field);
  }
  for (auto& field : *thriftLoadBalancerFields.mplsFields()) {
    mpls.insert(field);
  }

  auto fields = LoadBalancerFields(
      *thriftLoadBalancerFields.id(),
      *thriftLoadBalancerFields.algorithm(),
      static_cast<uint32_t>(*thriftLoadBalancerFields.seed()),
      v4,
      v6,
      tr,
      mpls);
  return fields;
}

folly::dynamic LoadBalancerFields::migrateToThrifty(folly::dynamic const& dyn) {
  folly::dynamic obj = dyn;
  auto seed = static_cast<int32_t>(obj["seed"].asInt());
  obj["seed"] = seed;
  return obj;
}

void LoadBalancerFields::migrateFromThrifty(folly::dynamic& dyn) {
  auto seed = dyn["seed"].asInt();
  dyn["seed"] = static_cast<uint32_t>(seed);
}

std::shared_ptr<LoadBalancer> LoadBalancer::fromThrift(
    const state::LoadBalancerFields& fields) {
  return std::make_shared<LoadBalancer>(fields);
}

template class ThriftStructNode<LoadBalancer, state::LoadBalancerFields>;

} // namespace facebook::fboss
