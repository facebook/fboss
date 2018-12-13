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
  config.ports[0].name = "port1";
  config.ports[1].logicalID = 2;
  config.ports[1].name = "port2";
  config.qosPolicies.resize(1);
  config.qosPolicies[0].name = "qp1";
  config.qosPolicies[0].rules = dscpRules({{0, {44, 45, 46}}});
  cfg::TrafficPolicyConfig trafficPolicy;
  config.__isset.dataPlaneTrafficPolicy = true;
  trafficPolicy.defaultQosPolicy = "qp1";
  trafficPolicy.__isset.defaultQosPolicy = true;
  config.dataPlaneTrafficPolicy = trafficPolicy;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  for (auto& portId : {1, 2}) {
    auto port = stateV1->getPort(PortID(portId));
    ASSERT_EQ("qp1", port->getQosPolicy().value());
  }
}
