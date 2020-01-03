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
#include "fboss/agent/Platform.h"
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
}; // namespace

namespace facebook::fboss {

LoadBalancerFields::LoadBalancerFields(
    LoadBalancerID id,
    cfg::HashingAlgorithm algorithm,
    uint32_t seed,
    LoadBalancerFields::IPv4Fields v4Fields,
    LoadBalancerFields::IPv6Fields v6Fields,
    LoadBalancerFields::TransportFields transportFields,
    LoadBalancerFields::MPLSFields mplsFields)
    : id_(id),
      algorithm_(algorithm),
      seed_(seed),
      v4Fields_(v4Fields),
      v6Fields_(v6Fields),
      transportFields_(transportFields),
      mplsFields_(mplsFields) {}

LoadBalancer::LoadBalancer(
    LoadBalancerID id,
    cfg::HashingAlgorithm algorithm,
    uint32_t seed,
    LoadBalancer::IPv4Fields v4Fields,
    LoadBalancer::IPv6Fields v6Fields,
    LoadBalancer::TransportFields transportFields,
    LoadBalancer::MPLSFields mplsFields)
    : NodeBaseT(
          id,
          algorithm,
          seed,
          v4Fields,
          v6Fields,
          transportFields,
          mplsFields) {}

LoadBalancerID LoadBalancer::getID() const {
  return getFields()->id_;
}

uint32_t LoadBalancer::getSeed() const {
  return getFields()->seed_;
}

cfg::HashingAlgorithm LoadBalancer::getAlgorithm() const {
  return getFields()->algorithm_;
}

LoadBalancer::IPv4FieldsRange LoadBalancer::getIPv4Fields() const {
  return folly::range(
      getFields()->v4Fields_.begin(), getFields()->v4Fields_.end());
}

LoadBalancer::IPv6FieldsRange LoadBalancer::getIPv6Fields() const {
  return folly::range(
      getFields()->v6Fields_.begin(), getFields()->v6Fields_.end());
}

LoadBalancer::TransportFieldsRange LoadBalancer::getTransportFields() const {
  return folly::range(
      getFields()->transportFields_.begin(),
      getFields()->transportFields_.end());
}

LoadBalancer::MPLSFieldsRange LoadBalancer::getMPLSFields() const {
  return folly::range(
      getFields()->mplsFields_.begin(), getFields()->mplsFields_.end());
}

std::shared_ptr<LoadBalancer> LoadBalancer::fromFollyDynamic(
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

  return std::make_shared<LoadBalancer>(
      id,
      algorithm,
      seed,
      std::move(ipv4Fields),
      std::move(ipv6Fields),
      std::move(transportFields),
      mplsFields);
}

folly::dynamic LoadBalancer::toFollyDynamic() const {
  folly::dynamic serialized = folly::dynamic::object();

  using IDType = std::underlying_type<LoadBalancerID>::type;
  serialized[kLoadBalancerID] = static_cast<IDType>(getID());

  using AlgorithmType = std::underlying_type<cfg::HashingAlgorithm>::type;
  serialized[kAlgorithm] = static_cast<AlgorithmType>(getAlgorithm());

  serialized[kSeed] = getSeed();

  serialized[kIPv4Fields] = folly::dynamic::array();
  for (const auto& ipv4Field : getIPv4Fields()) {
    serialized[kIPv4Fields].push_back(static_cast<int64_t>(ipv4Field));
  }

  serialized[kIPv6Fields] = folly::dynamic::array();
  for (const auto& ipv6Field : getIPv6Fields()) {
    serialized[kIPv6Fields].push_back(static_cast<int64_t>(ipv6Field));
  }

  serialized[kTransportFields] = folly::dynamic::array();
  for (const auto& transportField : getTransportFields()) {
    serialized[kTransportFields].push_back(
        static_cast<int64_t>(transportField));
  }

  serialized[kMPLSFields] = folly::dynamic::array();
  for (const auto& mplsField : getMPLSFields()) {
    serialized[kMPLSFields].push_back(static_cast<uint8_t>(mplsField));
  }
  return serialized;
}

uint32_t LoadBalancer::generateDeterministicSeed(
    LoadBalancerID loadBalancerID,
    const Platform* platform) {
  // To avoid changing the seed across graceful restarts, the seed is generated
  // deterministically using the local MAC address.
  auto mac64 = platform->getLocalMac().u64HBO();
  uint32_t mac32 = static_cast<uint32_t>(mac64 & 0xFFFFFFFF);

  uint32_t seed = 0;
  switch (loadBalancerID) {
    case LoadBalancerID::ECMP:
      seed = folly::hash::jenkins_rev_mix32(mac32);
      break;
    case LoadBalancerID::AGGREGATE_PORT:
      seed = folly::hash::twang_32from64(mac64);
      break;
  }

  return seed;
}

bool LoadBalancer::operator==(const LoadBalancer& rhs) const {
  return getID() == rhs.getID() && getSeed() == rhs.getSeed() &&
      getAlgorithm() == rhs.getAlgorithm() &&
      std::equal(
             getIPv4Fields().begin(),
             getIPv4Fields().end(),
             rhs.getIPv4Fields().begin(),
             rhs.getIPv4Fields().end()) &&
      std::equal(
             getIPv6Fields().begin(),
             getIPv6Fields().end(),
             rhs.getIPv6Fields().begin(),
             rhs.getIPv6Fields().end()) &&
      std::equal(
             getTransportFields().begin(),
             getTransportFields().end(),
             rhs.getTransportFields().begin(),
             rhs.getTransportFields().end()) &&
      std::equal(
             getMPLSFields().begin(),
             getMPLSFields().end(),
             rhs.getMPLSFields().begin(),
             rhs.getMPLSFields().end());
}

bool LoadBalancer::operator!=(const LoadBalancer& rhs) const {
  return !(*this == rhs);
}

// template class NodeBaseT<AggregatePort, AggregatePortFields>;

} // namespace facebook::fboss
