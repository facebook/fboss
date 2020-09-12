/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/LoadBalancer.h"
#include "fboss/agent/state/LoadBalancerMap.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/types.h"

#include <folly/MacAddress.h>
#include <folly/hash/Hash.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <set>

using namespace facebook::fboss;

namespace {

cfg::Fields halfFlow() {
  cfg::Fields fields;

  std::set<cfg::IPv4Field> v4Fields = {cfg::IPv4Field::SOURCE_ADDRESS,
                                       cfg::IPv4Field::DESTINATION_ADDRESS};
  fields.ipv4Fields_ref() = std::move(v4Fields);

  std::set<cfg::IPv6Field> v6Fields = {cfg::IPv6Field::SOURCE_ADDRESS,
                                       cfg::IPv6Field::DESTINATION_ADDRESS};
  fields.ipv6Fields_ref() = std::move(v6Fields);

  std::set<cfg::MPLSField> mplsFields = {LoadBalancer::MPLSField::TOP_LABEL,
                                         LoadBalancer::MPLSField::SECOND_LABEL,
                                         LoadBalancer::MPLSField::THIRD_LABEL};
  fields.mplsFields_ref() = std::move(mplsFields);
  return fields;
}

cfg::Fields fullFlow() {
  cfg::Fields fields = halfFlow();

  std::set<cfg::TransportField> transportFields = {
      cfg::TransportField::SOURCE_PORT, cfg::TransportField::DESTINATION_PORT};
  fields.transportFields_ref() = std::move(transportFields);

  return fields;
}

cfg::LoadBalancer defaultEcmpHash() {
  cfg::LoadBalancer ecmpDefault;

  ecmpDefault.id_ref() = cfg::LoadBalancerID::ECMP;
  ecmpDefault.fieldSelection_ref() = fullFlow();
  ecmpDefault.algorithm_ref() = cfg::HashingAlgorithm::CRC16_CCITT;

  return ecmpDefault;
}

cfg::LoadBalancer defaultLagHash() {
  cfg::LoadBalancer lagDefault;

  lagDefault.id_ref() = cfg::LoadBalancerID::AGGREGATE_PORT;
  lagDefault.fieldSelection_ref() = halfFlow();
  lagDefault.algorithm_ref() = cfg::HashingAlgorithm::CRC16_CCITT;

  return lagDefault;
}

std::vector<cfg::LoadBalancer> defaultLoadBalancers() {
  std::vector<cfg::LoadBalancer> loadBalancers = {defaultEcmpHash(),
                                                  defaultLagHash()};
  return loadBalancers;
}

void checkLoadBalancer(
    std::shared_ptr<LoadBalancer> loadBalancer,
    LoadBalancerID expectedID,
    cfg::HashingAlgorithm expectedAlgorithm,
    uint32_t expectedSeed,
    LoadBalancer::IPv4FieldsRange expectedV4Fields,
    LoadBalancer::IPv6FieldsRange expectedV6Fields,
    LoadBalancer::TransportFieldsRange expectedTransportFields,
    LoadBalancer::MPLSFieldsRange expectedMPLSFieldsRange) {
  ASSERT_NE(nullptr, loadBalancer);
  EXPECT_EQ(expectedID, loadBalancer->getID());
  EXPECT_EQ(expectedAlgorithm, loadBalancer->getAlgorithm());
  EXPECT_EQ(expectedSeed, loadBalancer->getSeed());
  EXPECT_TRUE(std::equal(
      expectedV4Fields.begin(),
      expectedV4Fields.end(),
      loadBalancer->getIPv4Fields().begin(),
      loadBalancer->getIPv4Fields().end()));
  EXPECT_TRUE(std::equal(
      expectedV6Fields.begin(),
      expectedV6Fields.end(),
      loadBalancer->getIPv6Fields().begin(),
      loadBalancer->getIPv6Fields().end()));
  EXPECT_TRUE(std::equal(
      expectedTransportFields.begin(),
      expectedTransportFields.end(),
      loadBalancer->getTransportFields().begin(),
      loadBalancer->getTransportFields().end()));
  EXPECT_TRUE(std::equal(
      expectedMPLSFieldsRange.begin(),
      expectedMPLSFieldsRange.end(),
      loadBalancer->getMPLSFields().begin(),
      loadBalancer->getMPLSFields().end()));
}

uint32_t generateDefaultEcmpSeed(const Platform* platform) {
  // This code previously lived in BcmSwitch::ecmpHashSetup(). In this way, it
  // will allow us to confirm that seeds aren't changed as we transition from
  // hard-coded load-balancing to configurable load-balancing.
  auto mac64 = platform->getLocalMac().u64HBO();
  uint32_t mac32 = static_cast<uint32_t>(mac64 & 0xFFFFFFFF);
  return folly::hash::jenkins_rev_mix32(mac32);
}

uint32_t generateDefaultLagSeed(const Platform* platform) {
  auto mac64 = platform->getLocalMac().u64HBO();
  return folly::hash::twang_32from64(mac64);
}

} // namespace

TEST(LoadBalancer, defaultConfiguration) {
  LoadBalancer::IPv4Fields v4SrcAndDst(
      {LoadBalancer::IPv4Field::SOURCE_ADDRESS,
       LoadBalancer::IPv4Field::DESTINATION_ADDRESS});

  LoadBalancer::IPv6Fields v6SrcAndDst(
      {LoadBalancer::IPv6Field::SOURCE_ADDRESS,
       LoadBalancer::IPv6Field::DESTINATION_ADDRESS});

  LoadBalancer::TransportFields transportSrcAndDst(
      {LoadBalancer::TransportField::SOURCE_PORT,
       LoadBalancer::TransportField::DESTINATION_PORT});

  LoadBalancer::MPLSFields mplsFields{LoadBalancer::MPLSField::TOP_LABEL,
                                      LoadBalancer::MPLSField::SECOND_LABEL,
                                      LoadBalancer::MPLSField::THIRD_LABEL};

  auto platform = createMockPlatform();
  auto initialState = std::make_shared<SwitchState>();

  cfg::SwitchConfig config;
  *config.loadBalancers_ref() = defaultLoadBalancers();

  auto finalState =
      publishAndApplyConfig(initialState, &config, platform.get());
  ASSERT_NE(nullptr, finalState);

  auto ecmpLoadBalancer =
      finalState->getLoadBalancers()->getLoadBalancerIf(LoadBalancerID::ECMP);
  ASSERT_NE(nullptr, ecmpLoadBalancer);
  checkLoadBalancer(
      ecmpLoadBalancer,
      LoadBalancerID::ECMP,
      cfg::HashingAlgorithm::CRC16_CCITT,
      generateDefaultEcmpSeed(platform.get()),
      folly::range(v4SrcAndDst.begin(), v4SrcAndDst.end()),
      folly::range(v6SrcAndDst.begin(), v6SrcAndDst.end()),
      folly::range(transportSrcAndDst.begin(), transportSrcAndDst.end()),
      folly::range(mplsFields.begin(), mplsFields.end()));

  auto lagLoadBalancer = finalState->getLoadBalancers()->getLoadBalancerIf(
      LoadBalancerID::AGGREGATE_PORT);
  ASSERT_NE(nullptr, lagLoadBalancer);
  checkLoadBalancer(
      lagLoadBalancer,
      LoadBalancerID::AGGREGATE_PORT,
      cfg::HashingAlgorithm::CRC16_CCITT,
      generateDefaultLagSeed(platform.get()),
      folly::range(v4SrcAndDst.begin(), v4SrcAndDst.end()),
      folly::range(v6SrcAndDst.begin(), v6SrcAndDst.end()),
      LoadBalancer::TransportFieldsRange(),
      folly::range(mplsFields.begin(), mplsFields.end()));
}

TEST(LoadBalancer, deserializationInverseOfSerlization) {
  auto origLoadBalancerID = LoadBalancerID::AGGREGATE_PORT;
  auto origHash = cfg::HashingAlgorithm::CRC16_CCITT;
  uint32_t origSeed = 0xA51C5EED;
  LoadBalancer::IPv4Fields origV4Src({LoadBalancer::IPv4Field::SOURCE_ADDRESS});
  LoadBalancer::IPv6Fields origV6Dst(
      {LoadBalancer::IPv6Field::DESTINATION_ADDRESS});
  LoadBalancer::TransportFields origTransportSrcAndDst(
      {LoadBalancer::TransportField::SOURCE_PORT,
       LoadBalancer::TransportField::DESTINATION_PORT});
  LoadBalancer::MPLSFields mplsFields{LoadBalancer::MPLSField::TOP_LABEL,
                                      LoadBalancer::MPLSField::SECOND_LABEL,
                                      LoadBalancer::MPLSField::THIRD_LABEL};

  LoadBalancer loadBalancer(
      origLoadBalancerID,
      origHash,
      origSeed,
      origV4Src,
      origV6Dst,
      origTransportSrcAndDst,
      mplsFields);

  auto serializedLoadBalancer = loadBalancer.toFollyDynamic();
  auto deserializedLoadBalancerPtr =
      LoadBalancer::fromFollyDynamic(serializedLoadBalancer);

  checkLoadBalancer(
      deserializedLoadBalancerPtr,
      origLoadBalancerID,
      origHash,
      origSeed,
      folly::range(origV4Src.begin(), origV4Src.end()),
      folly::range(origV6Dst.begin(), origV6Dst.end()),
      folly::range(
          origTransportSrcAndDst.begin(), origTransportSrcAndDst.end()),
      folly::range(mplsFields.begin(), mplsFields.end()));
}

namespace {

void checkLoadBalancersDelta(
    const std::shared_ptr<LoadBalancerMap>& oldLoadBalancers,
    const std::shared_ptr<LoadBalancerMap>& newLoadBalancers,
    const std::set<LoadBalancerID>& changedIDs,
    const std::set<LoadBalancerID>& addedIDs,
    const std::set<LoadBalancerID>& removedIDs) {
  auto oldState = std::make_shared<SwitchState>();
  oldState->resetLoadBalancers(oldLoadBalancers);
  auto newState = std::make_shared<SwitchState>();
  newState->resetLoadBalancers(newLoadBalancers);

  std::set<LoadBalancerID> foundChanged;
  std::set<LoadBalancerID> foundAdded;
  std::set<LoadBalancerID> foundRemoved;
  StateDelta delta(oldState, newState);
  DeltaFunctions::forEachChanged(
      delta.getLoadBalancersDelta(),
      [&](const std::shared_ptr<LoadBalancer>& oldLoadBalancer,
          const std::shared_ptr<LoadBalancer>& newLoadBalancer) {
        EXPECT_EQ(oldLoadBalancer->getID(), newLoadBalancer->getID());
        EXPECT_NE(oldLoadBalancer, newLoadBalancer);

        auto ret = foundChanged.insert(oldLoadBalancer->getID());
        EXPECT_TRUE(ret.second);
      },
      [&](const std::shared_ptr<LoadBalancer>& loadBalancer) {
        auto ret = foundAdded.insert(loadBalancer->getID());
        EXPECT_TRUE(ret.second);
      },
      [&](const std::shared_ptr<LoadBalancer>& loadBalancer) {
        auto ret = foundRemoved.insert(loadBalancer->getID());
        EXPECT_TRUE(ret.second);
      });

  EXPECT_EQ(changedIDs, foundChanged);
  EXPECT_EQ(addedIDs, foundAdded);
  EXPECT_EQ(removedIDs, foundRemoved);
}

} // namespace

/* The below tests are structured as follows:
 * baseState --[apply "baseConfig"]--> startState --[apply "config"]--> endState
 * In these tests, what's being tested is the transition from startState and
 * endSate.
 * baseConfig is used simply as a convenient way to transition baseState to
 * startState.
 */
TEST(LoadBalancerMap, idempotence) {
  LoadBalancer::IPv4Fields v4SrcAndDst(
      {LoadBalancer::IPv4Field::SOURCE_ADDRESS,
       LoadBalancer::IPv4Field::DESTINATION_ADDRESS});

  LoadBalancer::IPv6Fields v6SrcAndDst(
      {LoadBalancer::IPv6Field::SOURCE_ADDRESS,
       LoadBalancer::IPv6Field::DESTINATION_ADDRESS});

  LoadBalancer::TransportFields transportSrcAndDst(
      {LoadBalancer::TransportField::SOURCE_PORT,
       LoadBalancer::TransportField::DESTINATION_PORT});

  LoadBalancer::MPLSFields mplsFields{LoadBalancer::MPLSField::TOP_LABEL,
                                      LoadBalancer::MPLSField::SECOND_LABEL,
                                      LoadBalancer::MPLSField::THIRD_LABEL};

  auto platform = createMockPlatform();
  auto baseState = std::make_shared<SwitchState>();

  cfg::SwitchConfig baseConfig;
  *baseConfig.loadBalancers_ref() = defaultLoadBalancers();

  auto startState =
      publishAndApplyConfig(baseState, &baseConfig, platform.get());
  ASSERT_NE(nullptr, startState);

  // Applying the same config should be a no-op
  auto config = baseConfig;
  EXPECT_EQ(
      nullptr, publishAndApplyConfig(startState, &config, platform.get()));
}

TEST(LoadBalancerMap, addLoadBalancer) {
  auto platform = createMockPlatform();
  auto baseState = std::make_shared<SwitchState>();

  cfg::SwitchConfig baseConfig;
  std::vector<cfg::LoadBalancer> ecmpLoadBalancer = {defaultEcmpHash()};
  baseConfig.loadBalancers_ref() = ecmpLoadBalancer;

  auto startState =
      publishAndApplyConfig(baseState, &baseConfig, platform.get());
  ASSERT_NE(nullptr, startState);
  auto startLoadBalancers = startState->getLoadBalancers();
  ASSERT_NE(nullptr, startLoadBalancers);
  EXPECT_EQ(1, startLoadBalancers->size());

  // This config adds a DEFAULT_LAG_HASH LoadBalancer
  cfg::SwitchConfig config;
  *config.loadBalancers_ref() = defaultLoadBalancers();

  auto endState = publishAndApplyConfig(startState, &config, platform.get());
  ASSERT_NE(nullptr, endState);
  auto endLoadBalancers = endState->getLoadBalancers();
  ASSERT_NE(nullptr, endLoadBalancers);
  EXPECT_EQ(2, endLoadBalancers->getGeneration());
  EXPECT_EQ(2, endLoadBalancers->size());

  std::set<LoadBalancerID> updated = {};
  std::set<LoadBalancerID> added = {LoadBalancerID::AGGREGATE_PORT};
  std::set<LoadBalancerID> removed = {};
  checkLoadBalancersDelta(
      startLoadBalancers, endLoadBalancers, updated, added, removed);

  // Check LoadBalancerID::ECMP has not been modified
  EXPECT_EQ(
      startLoadBalancers->getLoadBalancerIf(LoadBalancerID::ECMP),
      endLoadBalancers->getLoadBalancerIf(LoadBalancerID::ECMP));
}

TEST(LoadBalancerMap, removeLoadBalancer) {
  auto platform = createMockPlatform();
  auto baseState = std::make_shared<SwitchState>();

  cfg::SwitchConfig baseConfig;
  *baseConfig.loadBalancers_ref() = defaultLoadBalancers();

  auto startState =
      publishAndApplyConfig(baseState, &baseConfig, platform.get());
  ASSERT_NE(nullptr, startState);
  auto startLoadBalancers = startState->getLoadBalancers();
  ASSERT_NE(nullptr, startLoadBalancers);
  EXPECT_EQ(2, startLoadBalancers->size());

  // This config removes the DEFAULT_LAG_HASH LoadBalancer
  cfg::SwitchConfig config;
  std::vector<cfg::LoadBalancer> ecmpLoadBalancer = {defaultEcmpHash()};
  config.loadBalancers_ref() = ecmpLoadBalancer;

  auto endState = publishAndApplyConfig(startState, &config, platform.get());
  ASSERT_NE(nullptr, endState);
  auto endLoadBalancers = endState->getLoadBalancers();
  ASSERT_NE(nullptr, endLoadBalancers);
  EXPECT_EQ(2, endLoadBalancers->getGeneration());
  EXPECT_EQ(1, endLoadBalancers->size());

  std::set<LoadBalancerID> updated = {};
  std::set<LoadBalancerID> added = {};
  std::set<LoadBalancerID> removed = {LoadBalancerID::AGGREGATE_PORT};
  checkLoadBalancersDelta(
      startLoadBalancers, endLoadBalancers, updated, added, removed);

  // Check LoadBalancerID::ECMP has not been modified
  EXPECT_EQ(
      startLoadBalancers->getLoadBalancerIf(LoadBalancerID::ECMP),
      endLoadBalancers->getLoadBalancerIf(LoadBalancerID::ECMP));
}

TEST(LoadBalancerMap, updateLoadBalancer) {
  auto platform = createMockPlatform();
  auto baseState = std::make_shared<SwitchState>();

  cfg::SwitchConfig baseConfig;
  *baseConfig.loadBalancers_ref() = defaultLoadBalancers();

  auto startState =
      publishAndApplyConfig(baseState, &baseConfig, platform.get());
  ASSERT_NE(nullptr, startState);
  auto startLoadBalancers = startState->getLoadBalancers();
  ASSERT_NE(nullptr, startLoadBalancers);
  EXPECT_EQ(2, startLoadBalancers->size());
  auto startEcmpLoadBalancer =
      startLoadBalancers->getLoadBalancerIf(LoadBalancerID::ECMP);
  ASSERT_NE(nullptr, startEcmpLoadBalancer);

  // This config modifies the DEFAULT_EMCP_HASH LoadBalancer
  cfg::SwitchConfig config;
  *config.loadBalancers_ref() = defaultLoadBalancers();
  // ECMP will now also use a half-hash
  *config.loadBalancers_ref()[0].fieldSelection_ref()->transportFields_ref() =
      {};

  auto endState = publishAndApplyConfig(startState, &config, platform.get());
  ASSERT_NE(nullptr, endState);
  auto endLoadBalancers = endState->getLoadBalancers();
  ASSERT_NE(nullptr, endLoadBalancers);
  EXPECT_EQ(2, endLoadBalancers->getGeneration());
  EXPECT_EQ(2, endLoadBalancers->size());

  std::set<LoadBalancerID> updated = {LoadBalancerID::ECMP};
  std::set<LoadBalancerID> added = {};
  std::set<LoadBalancerID> removed = {};
  checkLoadBalancersDelta(
      startLoadBalancers, endLoadBalancers, updated, added, removed);

  // Check LoadBalancerID::AGGREGATE_PORT has not been modified
  EXPECT_EQ(
      startLoadBalancers->getLoadBalancerIf(LoadBalancerID::AGGREGATE_PORT),
      endLoadBalancers->getLoadBalancerIf(LoadBalancerID::AGGREGATE_PORT));

  checkLoadBalancer(
      endLoadBalancers->getLoadBalancerIf(LoadBalancerID::ECMP),
      startEcmpLoadBalancer->getID(),
      startEcmpLoadBalancer->getAlgorithm(),
      startEcmpLoadBalancer->getSeed(),
      startEcmpLoadBalancer->getIPv4Fields(),
      startEcmpLoadBalancer->getIPv6Fields(),
      LoadBalancer::TransportFieldsRange(),
      startEcmpLoadBalancer->getMPLSFields());
}

TEST(LoadBalancerMap, deserializationInverseOfSerlization) {
  auto aggPortOrigLoadBalancerID = LoadBalancerID::AGGREGATE_PORT;
  auto aggPortOrigHash = cfg::HashingAlgorithm::CRC16_CCITT;
  uint32_t aggPortOrigSeed = 0xA51C5EED;
  LoadBalancer::IPv4Fields aggPortOrigV4Src(
      {LoadBalancer::IPv4Field::SOURCE_ADDRESS});
  LoadBalancer::IPv6Fields aggPortOrigV6Dst(
      {LoadBalancer::IPv6Field::DESTINATION_ADDRESS});
  LoadBalancer::TransportFields aggPortOrigTransportSrcAndDst(
      {LoadBalancer::TransportField::SOURCE_PORT,
       LoadBalancer::TransportField::DESTINATION_PORT});
  LoadBalancer::MPLSFields aggMplsFields{LoadBalancer::MPLSField::TOP_LABEL,
                                         LoadBalancer::MPLSField::SECOND_LABEL,
                                         LoadBalancer::MPLSField::THIRD_LABEL};

  auto ecmpOrigLoadBalancerID = LoadBalancerID::ECMP;
  auto ecmpOrigHash = cfg::HashingAlgorithm::CRC16_CCITT;
  uint32_t ecmpOrigSeed = 0xA51C5EED;
  LoadBalancer::IPv4Fields ecmpOrigV4Dst(
      {LoadBalancer::IPv4Field::DESTINATION_ADDRESS});
  LoadBalancer::IPv6Fields ecmpOrigV6Src(
      {LoadBalancer::IPv6Field::SOURCE_ADDRESS});
  LoadBalancer::TransportFields ecmpOrigTransportSrcAndDst(
      {LoadBalancer::TransportField::SOURCE_PORT,
       LoadBalancer::TransportField::DESTINATION_PORT});
  LoadBalancer::MPLSFields ecmpMplsFields{LoadBalancer::MPLSField::TOP_LABEL,
                                          LoadBalancer::MPLSField::SECOND_LABEL,
                                          LoadBalancer::MPLSField::THIRD_LABEL};

  LoadBalancerMap loadBalancerMap;
  loadBalancerMap.addLoadBalancer(std::make_shared<LoadBalancer>(
      aggPortOrigLoadBalancerID,
      aggPortOrigHash,
      aggPortOrigSeed,
      aggPortOrigV4Src,
      aggPortOrigV6Dst,
      aggPortOrigTransportSrcAndDst,
      aggMplsFields));
  loadBalancerMap.addLoadBalancer(std::make_shared<LoadBalancer>(
      ecmpOrigLoadBalancerID,
      ecmpOrigHash,
      ecmpOrigSeed,
      ecmpOrigV4Dst,
      ecmpOrigV6Src,
      ecmpOrigTransportSrcAndDst,
      ecmpMplsFields));

  auto serializedLoadBalancerMap = loadBalancerMap.toFollyDynamic();
  auto deserializedLoadBalancerMapPtr =
      LoadBalancerMap::fromFollyDynamic(serializedLoadBalancerMap);

  checkLoadBalancer(
      deserializedLoadBalancerMapPtr->getLoadBalancerIf(
          LoadBalancerID::AGGREGATE_PORT),
      aggPortOrigLoadBalancerID,
      aggPortOrigHash,
      aggPortOrigSeed,
      folly::range(aggPortOrigV4Src.begin(), aggPortOrigV4Src.end()),
      folly::range(aggPortOrigV6Dst.begin(), aggPortOrigV6Dst.end()),
      folly::range(
          aggPortOrigTransportSrcAndDst.begin(),
          aggPortOrigTransportSrcAndDst.end()),
      folly::range(aggMplsFields.begin(), aggMplsFields.end()));
  checkLoadBalancer(
      deserializedLoadBalancerMapPtr->getLoadBalancerIf(LoadBalancerID::ECMP),
      ecmpOrigLoadBalancerID,
      ecmpOrigHash,
      ecmpOrigSeed,
      folly::range(ecmpOrigV4Dst.begin(), ecmpOrigV4Dst.end()),
      folly::range(ecmpOrigV6Src.begin(), ecmpOrigV6Src.end()),
      folly::range(
          ecmpOrigTransportSrcAndDst.begin(), ecmpOrigTransportSrcAndDst.end()),
      folly::range(ecmpMplsFields.begin(), ecmpMplsFields.end()));
}
