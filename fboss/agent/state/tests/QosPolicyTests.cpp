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

void checkQosPolicy(
    const cfg::QosPolicy& cfgQosPolicy,
    std::shared_ptr<QosPolicy> swQosPolicy) {
  ASSERT_EQ(swQosPolicy->getName(), cfgQosPolicy.name);
  int numCfgQosRules = 0;
  for (const auto& cfgQosRule : cfgQosPolicy.rules) {
    numCfgQosRules += cfgQosRule.dscp.size();
  }
  ASSERT_EQ(swQosPolicy->getRules().size(), numCfgQosRules);

  std::unordered_map<uint8_t, uint8_t> dscpMapping;
  for (const auto& rule : cfgQosPolicy.rules) {
    for (const auto& dscp : rule.dscp) {
      dscpMapping[dscp] = rule.queueId;
    }
  }

  for (const auto& qosRule : swQosPolicy->getRules()) {
    ASSERT_EQ(qosRule.queueId, dscpMapping[qosRule.dscp]);
  }
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
