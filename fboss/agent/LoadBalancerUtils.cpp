// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/LoadBalancerUtils.h"

#include <folly/hash/Hash.h>

namespace facebook::fboss::utility {

namespace {
uint32_t generateDeterministicSeedSai(
    cfg::LoadBalancerID loadBalancerID,
    const folly::MacAddress& mac) {
  auto mac64 = mac.u64HBO();
  uint32_t mac32 = static_cast<uint32_t>(mac64 & 0xFFFFFFFF);
  uint32_t seed = 0;
  switch (loadBalancerID) {
    case cfg::LoadBalancerID::ECMP:
      seed = folly::hash::twang_32from64(mac64);
      break;
    case cfg::LoadBalancerID::AGGREGATE_PORT:
      seed = folly::hash::jenkins_rev_mix32(mac32);
      break;
  }
  return seed;
}

uint32_t generateDeterministicSeedNonSai(
    cfg::LoadBalancerID loadBalancerID,
    const folly::MacAddress& mac) {
  auto mac64 = mac.u64HBO();
  uint32_t mac32 = static_cast<uint32_t>(mac64 & 0xFFFFFFFF);

  uint32_t seed = 0;
  switch (loadBalancerID) {
    case cfg::LoadBalancerID::ECMP:
      seed = folly::hash::jenkins_rev_mix32(mac32);
      break;
    case cfg::LoadBalancerID::AGGREGATE_PORT:
      seed = folly::hash::twang_32from64(mac64);
      break;
  }
  return seed;
}

} // namespace

uint32_t generateDeterministicSeed(
    cfg::LoadBalancerID id,
    const folly::MacAddress& mac,
    bool sai) {
  if (sai) {
    return generateDeterministicSeedSai(id, mac);
  } else {
    return generateDeterministicSeedNonSai(id, mac);
  }
}

} // namespace facebook::fboss::utility
