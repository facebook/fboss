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

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmQosUtils.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

extern "C" {
#include <bcm/error.h>
#include <bcm/qos.h>
}

namespace facebook::fboss {

class BcmQosMapTest : public BcmTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::oneL3IntfNPortConfig(
        getHwSwitch(), {masterLogicalPortIds()[0], masterLogicalPortIds()[1]});
  }
};

TEST_F(BcmQosMapTest, BcmNumberOfQoSMaps) {
  auto setup = [this]() { applyNewConfig(initialConfig()); };

  auto verify = [this]() {
    auto mapIdsAndFlags = getBcmQosMapIdsAndFlags(getUnit());
    // In a previous SDK (6.4.10) we have seen extra QoS Maps show
    // up post warm boot. This is fixed 6.5.13 onwards. Assert so we
    // can catch any future breakages.
    int numEntries = mapIdsAndFlags.size();
    EXPECT_EQ(numEntries, 0);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmQosMapTest, BcmDscpMapWithRules) {
  auto setup = [this]() {
    auto config = initialConfig();

    config.qosPolicies.resize(1);
    config.qosPolicies[0].name = "qp";
    config.qosPolicies[0].rules.resize(8);
    for (auto i = 0; i < 8; i++) {
      config.qosPolicies[0].rules[i].dscp.resize(8);
      for (auto j = 0; j < 8; j++) {
        config.qosPolicies[0].rules[i].dscp[j] = 8 * i + j;
      }
    }
    cfg::TrafficPolicyConfig dataPlaneTrafficPolicy;
    dataPlaneTrafficPolicy.defaultQosPolicy_ref() = "qp";
    config.dataPlaneTrafficPolicy_ref() = dataPlaneTrafficPolicy;
    applyNewConfig(config);
  };
  auto verify = [this]() {
    auto mapIdsAndFlags = getBcmQosMapIdsAndFlags(getUnit());
    int numEntries = mapIdsAndFlags.size();
    EXPECT_EQ(numEntries, 3); // by default setup ingress & egress mpls qos maps
    for (auto mapIdAndFlag : mapIdsAndFlags) {
      auto mapId = mapIdAndFlag.first;
      auto flag = mapIdAndFlag.second;
      if ((flag & (BCM_QOS_MAP_INGRESS | BCM_QOS_MAP_L3)) ==
          (BCM_QOS_MAP_INGRESS | BCM_QOS_MAP_L3)) {
        int array_count = 0;
        bcm_qos_map_multi_get(getUnit(), flag, mapId, 0, nullptr, &array_count);
        EXPECT_EQ(
            (flag & (BCM_QOS_MAP_INGRESS | BCM_QOS_MAP_L3)),
            (BCM_QOS_MAP_INGRESS | BCM_QOS_MAP_L3));
        EXPECT_EQ(array_count, 64);
      }
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmQosMapTest, BcmAllQosMaps) {
  auto setup = [this]() {
    auto config = initialConfig();

    cfg::QosMap qosMap;
    qosMap.dscpMaps.resize(8);
    for (auto i = 0; i < 8; i++) {
      qosMap.dscpMaps[i].internalTrafficClass = i;
      for (auto j = 0; j < 8; j++) {
        qosMap.dscpMaps[i].fromDscpToTrafficClass.push_back(8 * i + j);
      }
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

    config.qosPolicies.resize(1);
    config.qosPolicies[0].name = "qp";
    config.qosPolicies[0].qosMap_ref() = qosMap;

    cfg::TrafficPolicyConfig dataPlaneTrafficPolicy;
    dataPlaneTrafficPolicy.defaultQosPolicy_ref() = "qp";
    config.dataPlaneTrafficPolicy_ref() = dataPlaneTrafficPolicy;
    applyNewConfig(config);
  };

  auto verify = [this]() {
    auto mapIdsAndFlags = getBcmQosMapIdsAndFlags(getUnit());
    int numEntries = mapIdsAndFlags.size();
    EXPECT_EQ(
        numEntries, 3); // 3 qos maps (ingress dscp, ingress mpls, egress mpls)
    for (auto mapIdAndFlag : mapIdsAndFlags) {
      auto mapId = mapIdAndFlag.first;
      auto flag = mapIdAndFlag.second;
      int array_count = 0;
      bcm_qos_map_multi_get(getUnit(), flag, mapId, 0, nullptr, &array_count);
      if ((flag & (BCM_QOS_MAP_INGRESS | BCM_QOS_MAP_L3)) ==
          (BCM_QOS_MAP_INGRESS | BCM_QOS_MAP_L3)) {
        EXPECT_EQ(array_count, 64);
      }
      if ((flag & (BCM_QOS_MAP_INGRESS | BCM_QOS_MAP_MPLS)) ==
          (BCM_QOS_MAP_INGRESS | BCM_QOS_MAP_MPLS)) {
        EXPECT_EQ(array_count, 8);
      }
      if ((flag & (BCM_QOS_MAP_EGRESS | BCM_QOS_MAP_MPLS)) ==
          (BCM_QOS_MAP_EGRESS | BCM_QOS_MAP_MPLS)) {
        EXPECT_EQ(array_count, 64);
        std::vector<bcm_qos_map_t> entries;
        entries.resize(array_count);
        bcm_qos_map_multi_get(
            getUnit(),
            flag,
            mapId,
            entries.size(),
            entries.data(),
            &array_count);
        // not returning any invalid or ghost entries now
        EXPECT_EQ(array_count, 48);
      }
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
