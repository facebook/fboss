/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/tests/BcmTest.h"

#include "fboss/agent/hw/bcm/BcmSwitch.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"

namespace {
using namespace facebook::fboss;

void addQosPolicyWithRules(cfg::SwitchConfig& cfg, const std::string& name) {
  cfg::QosPolicy qosPolicy;
  *qosPolicy.name_ref() = name;
  qosPolicy.rules_ref()->resize(8);
  for (auto i = 0; i < 8; i++) {
    *qosPolicy.rules_ref()[i].queueId_ref() = i;
    for (auto j = 0; j < 8; j++) {
      qosPolicy.rules_ref()[i].dscp_ref()->push_back(8 * i + j);
    }
  }
  cfg.qosPolicies_ref()->push_back(qosPolicy);
}

void addQosPolicyWithDscpMap(cfg::SwitchConfig& cfg, const std::string& name) {
  cfg::QosPolicy qosPolicy;
  *qosPolicy.name_ref() = name;
  qosPolicy.qosMap_ref() = cfg::QosMap();
  qosPolicy.qosMap_ref()->dscpMaps_ref()->resize(8);
  for (auto i = 0; i < 8; i++) {
    *qosPolicy.qosMap_ref()->dscpMaps_ref()[i].internalTrafficClass_ref() = i;
    for (auto j = 0; j < 8; j++) {
      qosPolicy.qosMap_ref()
          ->dscpMaps_ref()[i]
          .fromDscpToTrafficClass_ref()
          ->push_back(8 * i + j);
    }
  }
  cfg.qosPolicies_ref()->push_back(qosPolicy);
}

void addQosPolicyWithDscpAndExpMap(
    cfg::SwitchConfig& cfg,
    const std::string& name) {
  cfg::QosPolicy qosPolicy;
  *qosPolicy.name_ref() = name;
  qosPolicy.qosMap_ref() = cfg::QosMap();
  qosPolicy.qosMap_ref()->dscpMaps_ref()->resize(8);
  qosPolicy.qosMap_ref()->expMaps_ref()->resize(8);
  for (auto i = 0; i < 8; i++) {
    // dscp map
    *qosPolicy.qosMap_ref()->dscpMaps_ref()[i].internalTrafficClass_ref() = i;
    for (auto j = 0; j < 8; j++) {
      qosPolicy.qosMap_ref()
          ->dscpMaps_ref()[i]
          .fromDscpToTrafficClass_ref()
          ->push_back(8 * i + j);
    }
    // exp map
    *qosPolicy.qosMap_ref()->expMaps_ref()[i].internalTrafficClass_ref() = i;
    qosPolicy.qosMap_ref()
        ->expMaps_ref()[i]
        .fromExpToTrafficClass_ref()
        ->push_back(i);
    qosPolicy.qosMap_ref()->expMaps_ref()[i].fromTrafficClassToExp_ref() = i;
  }
  cfg.qosPolicies_ref()->push_back(qosPolicy);
}

void setDataPlaneTrafficPolicy(
    cfg::SwitchConfig& cfg,
    const std::string& name) {
  cfg::TrafficPolicyConfig trafficPolicy;

  trafficPolicy.defaultQosPolicy_ref() = name;
  cfg.dataPlaneTrafficPolicy_ref() = trafficPolicy;
}

void setDataPlaneTrafficPolicyForPort(
    cfg::SwitchConfig& cfg,
    const std::string& name,
    int port) {
  if (auto policy = cfg.dataPlaneTrafficPolicy_ref()) {
    if (auto portIdToQosPolicy = policy->portIdToQosPolicy_ref()) {
      portIdToQosPolicy->emplace(port, name);
    } else {
      policy->portIdToQosPolicy_ref() = {{port, name}};
    }
  }
}

} // namespace
namespace facebook::fboss {

TEST_F(BcmTest, addPortFails) {
  const auto& portMap = getProgrammedState()->getPorts();
  auto highestPortIdPort = *std::max_element(
      portMap->begin(),
      portMap->end(),
      [=](const auto& lport, const auto& rport) {
        return lport->getID() < rport->getID();
      });
  auto newState = getProgrammedState()->clone();
  auto newPortMap = newState->getPorts()->modify(&newState);
  newPortMap->addPort(
      std::make_shared<Port>(PortID(highestPortIdPort->getID() + 1), "foo"));
  EXPECT_THROW(applyNewState(newState), FbossError);
}

TEST_F(BcmTest, removePortFails) {
  const auto& portMap = getProgrammedState()->getPorts();
  auto firstPort = *portMap->begin();
  auto newState = getProgrammedState()->clone();
  auto newPortMap = newState->getPorts()->modify(&newState);
  newPortMap->removeNode(firstPort->getID());
  EXPECT_THROW(applyNewState(newState), FbossError);
}

TEST_F(BcmTest, validQosPolicyConfigWithRules) {
  auto cfg =
      utility::onePortPerVlanConfig(getHwSwitch(), masterLogicalPortIds());
  auto state0 = applyNewConfig(cfg);
  addQosPolicyWithRules(cfg, "qp");
  setDataPlaneTrafficPolicy(cfg, "qp");
  auto newState = applyNewConfig(cfg);
  EXPECT_TRUE(getHwSwitch()->isValidStateUpdate(StateDelta(state0, newState)));
}

TEST_F(BcmTest, validQosPolicyConfigWithRulesAndDifferentPolicyForPort) {
  auto cfg =
      utility::onePortPerVlanConfig(getHwSwitch(), masterLogicalPortIds());
  auto state0 = applyNewConfig(cfg);
  addQosPolicyWithRules(cfg, "qp0");
  setDataPlaneTrafficPolicy(cfg, "qp0");
  addQosPolicyWithRules(cfg, "qp1");
  setDataPlaneTrafficPolicyForPort(
      cfg, "qp1", *cfg.ports_ref()[0].logicalID_ref());
  auto newState = applyNewConfig(cfg);
  EXPECT_TRUE(getHwSwitch()->isValidStateUpdate(StateDelta(state0, newState)));
}

TEST_F(BcmTest, expAndDscpQosMapDefaultQosPolicy) {
  auto cfg =
      utility::onePortPerVlanConfig(getHwSwitch(), masterLogicalPortIds());
  auto state0 = applyNewConfig(cfg);
  addQosPolicyWithDscpAndExpMap(cfg, "qp");
  setDataPlaneTrafficPolicy(cfg, "qp");
  auto newState = applyNewConfig(cfg);
  EXPECT_TRUE(getHwSwitch()->isValidStateUpdate(StateDelta(state0, newState)));
}

TEST_F(BcmTest, dscpQosMapForPort) {
  auto cfg =
      utility::onePortPerVlanConfig(getHwSwitch(), masterLogicalPortIds());
  auto state0 = applyNewConfig(cfg);
  addQosPolicyWithDscpAndExpMap(cfg, "qp0");
  setDataPlaneTrafficPolicy(cfg, "qp0");
  addQosPolicyWithDscpMap(cfg, "qp1");
  setDataPlaneTrafficPolicyForPort(
      cfg, "qp1", *cfg.ports_ref()[0].logicalID_ref());
  auto newState = applyNewConfig(cfg);
  EXPECT_TRUE(getHwSwitch()->isValidStateUpdate(StateDelta(state0, newState)));
}

TEST_F(BcmTest, expQosMapForPort) {
  auto cfg =
      utility::onePortPerVlanConfig(getHwSwitch(), masterLogicalPortIds());
  auto state0 = applyNewConfig(cfg);
  addQosPolicyWithDscpMap(cfg, "qp0");
  setDataPlaneTrafficPolicy(cfg, "qp0");
  addQosPolicyWithDscpAndExpMap(cfg, "qp1");
  setDataPlaneTrafficPolicyForPort(
      cfg, "qp1", *cfg.ports_ref()[0].logicalID_ref());
  auto newState = applyNewConfig(cfg);
  EXPECT_FALSE(getHwSwitch()->isValidStateUpdate(StateDelta(state0, newState)));
}

} // namespace facebook::fboss
