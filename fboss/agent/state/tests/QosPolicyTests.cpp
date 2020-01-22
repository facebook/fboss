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
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/QosPolicy.h"
#include "fboss/agent/state/QosPolicyMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <boost/container/flat_map.hpp>
#include <gtest/gtest.h>

using namespace facebook::fboss;
using std::make_shared;
using std::shared_ptr;

namespace {

std::vector<cfg::QosRule> dscpRules(
    std::vector<std::pair<uint8_t, std::vector<int16_t>>> rules) {
  std::vector<cfg::QosRule> dscpRules;
  for (const auto& pair : rules) {
    cfg::QosRule r;
    r.queueId = pair.first;
    r.dscp = pair.second;
    dscpRules.push_back(r);
  }
  return dscpRules;
}

void checkQosPolicyQosRules(
    const cfg::QosPolicy& cfgQosPolicy,
    std::shared_ptr<QosPolicy> swQosPolicy) {
  ASSERT_EQ(swQosPolicy->getName(), cfgQosPolicy.name);
  auto swStateFromTcToDscp = swQosPolicy->getDscpMap().from();
  DscpMap cfgDscpMap;
  for (const auto& rule : cfgQosPolicy.rules) {
    for (auto dscp : rule.dscp) {
      cfgDscpMap.addFromEntry(
          static_cast<TrafficClass>(rule.queueId), static_cast<DSCP>(dscp));
    }
  }
  ASSERT_EQ(swStateFromTcToDscp, cfgDscpMap.from());
}

void checkQosPolicy(
    const cfg::QosPolicy& cfgQosPolicy,
    std::shared_ptr<QosPolicy> swQosPolicy) {
  ASSERT_EQ(swQosPolicy->getName(), cfgQosPolicy.name);
  if (!cfgQosPolicy.rules.empty()) {
    return checkQosPolicyQosRules(cfgQosPolicy, swQosPolicy);
  }
  // use either rules or qosMap
  const auto& qosMap = cfgQosPolicy.qosMap_ref().value_unchecked();
  ASSERT_EQ(swQosPolicy->getDscpMap(), DscpMap(qosMap.dscpMaps));
  ASSERT_EQ(swQosPolicy->getExpMap(), ExpMap(qosMap.expMaps));

  std::set<std::pair<uint16_t, uint16_t>> cfgQueueMapEntries;
  std::set<std::pair<uint16_t, uint16_t>> swStateQueueMapEntries;
  for (auto entry : qosMap.trafficClassToQueueId) {
    cfgQueueMapEntries.emplace(entry.first, entry.second);
  }
  for (auto entry : swQosPolicy->getTrafficClassToQueueId()) {
    swStateQueueMapEntries.emplace(entry.first, entry.second);
  }
  EXPECT_EQ(cfgQueueMapEntries, swStateQueueMapEntries);
}

void checkQosSwState(
    cfg::SwitchConfig& config,
    std::shared_ptr<SwitchState> state) {
  auto cfgQosPolicies = config.qosPolicies;
  EXPECT_NE(nullptr, state);
  ASSERT_EQ(cfgQosPolicies.size(), state->getQosPolicies()->size());

  for (const auto& cfgQosPolicy : cfgQosPolicies) {
    auto swQosPolicy = state->getQosPolicy(cfgQosPolicy.name);
    checkQosPolicy(cfgQosPolicy, swQosPolicy);
  }
}

void checkDelta(
    const shared_ptr<QosPolicyMap>& oldQosPolicies,
    const shared_ptr<QosPolicyMap>& newQosPolicies,
    const std::set<std::string>& changedIDs,
    const std::set<std::string>& addedIDs,
    const std::set<std::string>& removedIDs) {
  auto oldState = make_shared<SwitchState>();
  oldState->resetQosPolicies(oldQosPolicies);
  auto newState = make_shared<SwitchState>();
  newState->resetQosPolicies(newQosPolicies);

  std::set<std::string> foundChanged;
  std::set<std::string> foundAdded;
  std::set<std::string> foundRemoved;
  StateDelta delta(oldState, newState);

  DeltaFunctions::forEachChanged(
      delta.getQosPoliciesDelta(),
      [&](const shared_ptr<QosPolicy>& oldQosPolicy,
          const shared_ptr<QosPolicy>& newQosPolicy) {
        EXPECT_EQ(oldQosPolicy->getID(), newQosPolicy->getID());
        EXPECT_NE(oldQosPolicy, newQosPolicy);

        auto ret = foundChanged.insert(oldQosPolicy->getID());
        EXPECT_TRUE(ret.second);
      },
      [&](const shared_ptr<QosPolicy>& qosPolicy) {
        auto ret = foundAdded.insert(qosPolicy->getID());
        EXPECT_TRUE(ret.second);
      },
      [&](const shared_ptr<QosPolicy>& qosPolicy) {
        auto ret = foundRemoved.insert(qosPolicy->getID());
        EXPECT_TRUE(ret.second);
      });

  EXPECT_EQ(changedIDs, foundChanged);
  EXPECT_EQ(addedIDs, foundAdded);
  EXPECT_EQ(removedIDs, foundRemoved);
}

cfg::QosMap cfgQosMap() {
  cfg::QosMap qosMap;
  qosMap.dscpMaps.resize(8);
  qosMap.expMaps.resize(8);
  for (auto i = 0; i < 8; i++) {
    auto& dscpMap = qosMap.dscpMaps[i];
    auto& expMap = qosMap.expMaps[i];
    dscpMap.internalTrafficClass = i;
    expMap.internalTrafficClass = i;
    for (auto j = 0; j < 4; j++) {
      dscpMap.fromDscpToTrafficClass.push_back(j);
    }
    expMap.fromExpToTrafficClass.push_back(i);
    dscpMap.fromTrafficClassToDscp_ref() = dscpMap.internalTrafficClass;
    expMap.fromTrafficClassToExp_ref() = expMap.internalTrafficClass;
    qosMap.trafficClassToQueueId.emplace(i, i);
  }
  return qosMap;
}

} // namespace

TEST(QosPolicy, AddSinglePolicy) {
  cfg::SwitchConfig config;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::QosPolicy p1;
  p1.name = "qosPolicy_1";
  p1.rules = dscpRules({{7, {46}}});
  config.qosPolicies.push_back(p1);
  auto state = publishAndApplyConfig(stateV0, &config, platform.get());

  checkQosSwState(config, state);
}

TEST(QosPolicy, AddMultiplePolicies) {
  cfg::SwitchConfig config;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::QosPolicy p1, p2;
  p1.name = "qosPolicy_1";
  p1.rules = dscpRules({{7, {45, 46}}});
  config.qosPolicies.push_back(p1);
  p2.name = "qosPolicy_2";
  p2.rules = dscpRules({{7, {45, 46}}, {1, {44}}});
  config.qosPolicies.push_back(p2);
  auto state = publishAndApplyConfig(stateV0, &config, platform.get());

  checkQosSwState(config, state);
}

TEST(QosPolicy, SerializePolicies) {
  cfg::SwitchConfig config;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::QosPolicy p1, p2;
  p1.name = "qosPolicy_1";
  p1.rules = dscpRules({{7, {13}}});
  config.qosPolicies.push_back(p1);
  p2.name = "qosPolicy_2";
  p2.rules = dscpRules({{7, {46}}, {1, {44, 45}}});
  config.qosPolicies.push_back(p2);
  auto state = publishAndApplyConfig(stateV0, &config, platform.get());

  auto qosPolicies = state->getQosPolicies();
  auto serialized = qosPolicies->toFollyDynamic();
  auto qosPoliciesBack = QosPolicyMap::fromFollyDynamic(serialized);

  EXPECT_TRUE(qosPolicies->size() == qosPoliciesBack->size());
  for (const auto& name : {"qosPolicy_1", "qosPolicy_2"}) {
    auto q1 = qosPolicies->getQosPolicyIf(name);
    auto q2 = qosPoliciesBack->getQosPolicyIf(name);
    EXPECT_NE(nullptr, q1);
    EXPECT_NE(nullptr, q2);
    EXPECT_TRUE(*q1 == *q2);
  }
}

TEST(QosPolicy, SerializePoliciesWithMap) {
  cfg::SwitchConfig config;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::QosPolicy p1, p2;
  p1.name = "qosPolicy_1";
  p2.name = "qosPolicy_2";

  cfg::QosMap qosMap;
  qosMap.dscpMaps.resize(8);
  for (auto i = 0; i < 8; i++) {
    qosMap.dscpMaps[i].internalTrafficClass = i;
    for (auto j = 0; j < 8; j++) {
      qosMap.dscpMaps[i].fromDscpToTrafficClass.push_back(8 * i + j);
    }
    qosMap.dscpMaps[i].fromTrafficClassToDscp_ref() = i;
  }
  qosMap.expMaps.resize(8);
  for (auto i = 0; i < 8; i++) {
    qosMap.expMaps[i].internalTrafficClass = i;
    qosMap.expMaps[i].fromExpToTrafficClass.push_back(i);
    qosMap.expMaps[i].fromTrafficClassToExp_ref() = i;
  }

  for (auto i = 0; i < 8; i++) {
    qosMap.trafficClassToQueueId.emplace(i, i);
  }
  p1.qosMap_ref() = qosMap;
  p2.qosMap_ref() = qosMap;

  config.qosPolicies.push_back(p1);
  config.qosPolicies.push_back(p2);
  auto state = publishAndApplyConfig(stateV0, &config, platform.get());

  auto qosPolicies = state->getQosPolicies();
  auto serialized = qosPolicies->toFollyDynamic();
  auto qosPoliciesBack = QosPolicyMap::fromFollyDynamic(serialized);

  EXPECT_TRUE(qosPolicies->size() == qosPoliciesBack->size());
  for (const auto& name : {"qosPolicy_1", "qosPolicy_2"}) {
    auto q1 = qosPolicies->getQosPolicyIf(name);
    auto q2 = qosPoliciesBack->getQosPolicyIf(name);
    EXPECT_NE(nullptr, q1);
    EXPECT_NE(nullptr, q2);
    EXPECT_TRUE(*q1 == *q2);
  }
}

TEST(QosPolicy, EmptyRules) {
  cfg::SwitchConfig config;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::QosPolicy p1;
  p1.name = "qosPolicy_1";
  p1.rules = dscpRules({{7, {}}});
  config.qosPolicies.push_back(p1);

  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &config, platform.get()), FbossError);
}

TEST(QosPolicy, PolicyConflict) {
  cfg::SwitchConfig config;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::QosPolicy p1, p2;
  p1.name = "qosPolicy_1";
  p1.rules = dscpRules({{7, {46}}});
  config.qosPolicies.push_back(p1);
  // Both policies have the same name
  p2.name = "qosPolicy_1";
  p2.rules = dscpRules({{7, {46}}, {1, {44, 45}}});
  config.qosPolicies.push_back(p2);

  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &config, platform.get()), FbossError);
}

TEST(QosPolicy, PolicyInvalidDscpValues) {
  cfg::SwitchConfig config;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::QosPolicy p1;
  p1.name = "qosPolicy_1";
  p1.rules = dscpRules({{7, {44, 255}}});
  config.qosPolicies.push_back(p1);
  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &config, platform.get()), FbossError);
}

TEST(QosPolicy, PortDefaultQosPolicy) {
  cfg::SwitchConfig config;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  stateV0->registerPort(PortID(1), "port1");
  stateV0->registerPort(PortID(2), "port2");

  config.ports.resize(2);
  config.ports[0].logicalID = 1;
  config.ports[0].name_ref() = "port1";
  config.ports[1].logicalID = 2;
  config.ports[1].name_ref() = "port2";
  config.qosPolicies.resize(1);
  config.qosPolicies[0].name = "qp1";
  config.qosPolicies[0].rules = dscpRules({{0, {44, 45, 46}}});
  cfg::TrafficPolicyConfig trafficPolicy;
  trafficPolicy.defaultQosPolicy_ref() = "qp1";
  config.dataPlaneTrafficPolicy_ref() = trafficPolicy;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  for (auto& portId : {1, 2}) {
    auto port = stateV1->getPort(PortID(portId));
    ASSERT_EQ("qp1", port->getQosPolicy().value());
  }
}

TEST(QosPolicy, PortQosPolicyOverride) {
  cfg::SwitchConfig config;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  stateV0->registerPort(PortID(1), "port1");
  stateV0->registerPort(PortID(2), "port2");

  config.ports.resize(2);
  config.ports[0].logicalID = 1;
  config.ports[0].name_ref() = "port1";
  config.ports[1].logicalID = 2;
  config.ports[1].name_ref() = "port2";
  config.qosPolicies.resize(2);
  config.qosPolicies[0].name = "qp1";
  config.qosPolicies[0].rules = dscpRules({{0, {46}}});
  config.qosPolicies[1].name = "qp2";
  config.qosPolicies[1].rules = dscpRules({{1, {46}}});
  cfg::TrafficPolicyConfig trafficPolicy;
  trafficPolicy.defaultQosPolicy_ref() = "qp1";
  trafficPolicy.portIdToQosPolicy_ref() = {{1, "qp2"}};
  config.dataPlaneTrafficPolicy_ref() = trafficPolicy;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_EQ("qp2", stateV1->getPort(PortID(1))->getQosPolicy().value());
  ASSERT_EQ("qp1", stateV1->getPort(PortID(2))->getQosPolicy().value());
}

TEST(QosPolicy, QosPolicyDelta) {
  cfg::SwitchConfig config;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::QosPolicy p1, p2;
  p1.name = "qosPolicy_1";
  p1.rules = dscpRules({{7, {46}}});
  config.qosPolicies.push_back(p1);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  auto qosPoliciesV1 = stateV1->getQosPolicies();

  /* Change 1 Policy */
  auto& p = config.qosPolicies.back();
  p.rules = dscpRules({{7, {45}}});
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  auto qosPoliciesV2 = stateV2->getQosPolicies();
  checkDelta(qosPoliciesV1, qosPoliciesV2, {"qosPolicy_1"}, {}, {});

  /* Add 1 Policy */
  p2.name = "qosPolicy_2";
  p2.rules = dscpRules({{2, {36}}, {3, {34, 35}}});
  config.qosPolicies.push_back(p2);
  auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
  auto qosPoliciesV3 = stateV3->getQosPolicies();
  checkDelta(qosPoliciesV2, qosPoliciesV3, {}, {"qosPolicy_2"}, {});

  /* Remove 1 Policy */
  config.qosPolicies[0] = config.qosPolicies[1];
  config.qosPolicies.pop_back();
  auto stateV4 = publishAndApplyConfig(stateV3, &config, platform.get());
  auto qosPoliciesV4 = stateV4->getQosPolicies();
  checkDelta(qosPoliciesV3, qosPoliciesV4, {}, {}, {"qosPolicy_1"});
}

TEST(QosPolicy, QosMap) {
  cfg::SwitchConfig config;
  auto platform = createMockPlatform();
  auto state = make_shared<SwitchState>();

  config.qosPolicies.resize(1);
  auto& policy = config.qosPolicies[0];
  policy.name = "qosPolicy";
  policy.qosMap_ref() = cfgQosMap();
  state = publishAndApplyConfig(state, &config, platform.get());
  checkQosPolicy(policy, state->getQosPolicy("qosPolicy"));

  // modiify qos map
  policy.qosMap_ref()->dscpMaps.pop_back();
  policy.qosMap_ref()->expMaps.pop_back();
  state = publishAndApplyConfig(state, &config, platform.get());
  checkQosPolicy(policy, state->getQosPolicy("qosPolicy"));

  // remove expMaps altogether
  policy.qosMap_ref()->expMaps.clear();
  state = publishAndApplyConfig(state, &config, platform.get());
  checkQosPolicy(policy, state->getQosPolicy("qosPolicy"));

  // allow both qos rule or maps
  policy.rules = dscpRules({{2, {36}}, {3, {34, 35}}});
  EXPECT_NO_THROW(publishAndApplyConfig(state, &config, platform.get()));
}

TEST(QosPolicy, DefaultQosPolicy) {
  cfg::SwitchConfig config;
  auto platform = createMockPlatform();
  auto state = make_shared<SwitchState>();

  config.qosPolicies.resize(1);
  auto& policy = config.qosPolicies[0];
  policy.name = "qosPolicy";
  policy.qosMap_ref() = cfgQosMap();
  state = publishAndApplyConfig(state, &config, platform.get());
  checkQosPolicy(policy, state->getQosPolicy("qosPolicy"));
  // default policy is not set
  EXPECT_EQ(state->getDefaultDataPlaneQosPolicy(), nullptr);

  // set default policy
  cfg::TrafficPolicyConfig defaultQosPolicy;
  defaultQosPolicy.defaultQosPolicy_ref() = "qosPolicy";
  config.dataPlaneTrafficPolicy_ref() = defaultQosPolicy;
  state = publishAndApplyConfig(state, &config, platform.get());

  EXPECT_NE(state->getDefaultDataPlaneQosPolicy(), nullptr);
  checkQosPolicy(policy, state->getDefaultDataPlaneQosPolicy());
  EXPECT_EQ(state->getQosPolicy("qosPolicy"), nullptr);
}

TEST(QosPolicy, DefaultQosPolicyOnPorts) {
  cfg::SwitchConfig config;
  auto platform = createMockPlatform();
  auto state = make_shared<SwitchState>();
  state->registerPort(PortID(1), "port1");
  state->registerPort(PortID(2), "port2");

  config.ports.resize(2);
  config.ports[0].logicalID = 1;
  config.ports[0].name_ref() = "port1";
  config.ports[1].logicalID = 2;
  config.ports[1].name_ref() = "port2";

  config.qosPolicies.resize(1);
  auto& policy = config.qosPolicies[0];
  policy.name = "qosPolicy";
  policy.qosMap_ref() = cfgQosMap();
  cfg::TrafficPolicyConfig defaultQosPolicy;
  defaultQosPolicy.defaultQosPolicy_ref() = "qosPolicy";
  config.dataPlaneTrafficPolicy_ref() = defaultQosPolicy;
  state = publishAndApplyConfig(state, &config, platform.get());

  checkQosPolicy(policy, state->getDefaultDataPlaneQosPolicy());
  for (auto& portId : {1, 2}) {
    auto port = state->getPort(PortID(portId));
    ASSERT_EQ("qosPolicy", port->getQosPolicy().value());
  }
}

TEST(QosPolicy, QosPolicyPortOverride) {
  cfg::SwitchConfig config;
  auto platform = createMockPlatform();
  auto state = make_shared<SwitchState>();
  state->registerPort(PortID(1), "port1");
  state->registerPort(PortID(2), "port2");

  config.ports.resize(2);
  config.ports[0].logicalID = 1;
  config.ports[0].name_ref() = "port1";
  config.ports[1].logicalID = 2;
  config.ports[1].name_ref() = "port2";

  config.qosPolicies.resize(2);
  auto& policy0 = config.qosPolicies[0];
  policy0.name = "qosPolicy0";
  policy0.qosMap_ref() = cfgQosMap();

  auto& policy1 = config.qosPolicies[1];
  policy1.name = "qosPolicy1";
  policy1.qosMap_ref() = cfgQosMap();
  policy1.qosMap_ref()->expMaps.clear();

  cfg::TrafficPolicyConfig defaultQosPolicy;
  defaultQosPolicy.defaultQosPolicy_ref() = "qosPolicy0";
  defaultQosPolicy.portIdToQosPolicy_ref() = {{1, "qosPolicy1"}};
  config.dataPlaneTrafficPolicy_ref() = defaultQosPolicy;

  state = publishAndApplyConfig(state, &config, platform.get());

  checkQosPolicy(policy0, state->getDefaultDataPlaneQosPolicy());
  checkQosPolicy(policy1, state->getQosPolicy("qosPolicy1"));

  const auto port0 = state->getPort(PortID(1));
  ASSERT_EQ("qosPolicy1", port0->getQosPolicy().value());
  const auto port1 = state->getPort(PortID(2));
  ASSERT_EQ("qosPolicy0", port1->getQosPolicy().value());
}

TEST(QosPolicy, QosPolicyConfigMigrateNoDelta) {
  cfg::SwitchConfig config;
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  std::vector<std::pair<uint8_t, std::vector<int16_t>>> rules{
      {0, {6, 7, 8, 9, 12, 13, 14, 15}},
      {1, {0,  1,  2,  3,  4,  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 28,
           29, 30, 31, 33, 34, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46,
           47, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63}},
      {2, {5}},
      {4, {10, 11}},
      {6, {32, 35, 26, 27}},
      {7, {46}}};
  cfg::QosPolicy p1;
  p1.name = "qosPolicy_1";
  p1.rules = dscpRules(rules);
  config.qosPolicies.push_back(p1);
  cfg::TrafficPolicyConfig policy;
  policy.defaultQosPolicy_ref() = "qosPolicy_1";
  config.dataPlaneTrafficPolicy_ref() = policy;
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());

  cfg::QosPolicy p2;
  p2.name = "qosPolicy_1";
  cfg::QosMap qosMap;
  for (const auto& entry : rules) {
    cfg::DscpQosMap dscpMap;
    dscpMap.internalTrafficClass = entry.first;
    for (auto dscp : entry.second) {
      dscpMap.fromDscpToTrafficClass.push_back(dscp);
    }
    qosMap.dscpMaps.push_back(dscpMap);
  }

  for (auto i = 0; i < 8; i++) {
    cfg::ExpQosMap expMap;
    expMap.internalTrafficClass = i;
    expMap.fromExpToTrafficClass.push_back(i);
    expMap.fromTrafficClassToExp_ref() = i;
    qosMap.expMaps.push_back(expMap);
    qosMap.trafficClassToQueueId.emplace(i, i);
  }
  p2.qosMap_ref() = qosMap;
  config.qosPolicies[0] = p2;
  auto stateV2 = publishAndApplyConfig(stateV0, &config, platform.get());

  StateDelta delta(stateV1, stateV2);
  const auto& policyDelta = delta.getQosPoliciesDelta();
  ASSERT_EQ(policyDelta.begin(), policyDelta.end());
}
