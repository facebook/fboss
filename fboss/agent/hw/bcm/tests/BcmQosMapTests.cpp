/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmQosPolicy.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmQosUtils.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

extern "C" {
#include <bcm/cosq.h>
#include <bcm/qos.h>
}

namespace {
// arbit vector values for testing
const std::vector<int> kTrafficClassToPgId{0, 1, 2, 3, 4, 5, 6, 7};
const std::vector<int> kPfcPriorityToPgId{7, 7, 7, 7, 7, 7, 7, 7};
const std::vector<int> kPfcPriorityToQueue = {0, 1, 2, 2, 4, 7, 7, 6};
// this one is an extenion of the kTrafficClassToPgId
// in HW kTrafficClassToPgId is programmed as 16 entries
const std::vector<int>
    kTrafficClassToPgIdInHw{0, 1, 2, 3, 4, 5, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7};
} // namespace

namespace facebook::fboss {

class BcmQosMapTest : public BcmTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::oneL3IntfNPortConfig(
        getHwSwitch()->getPlatform()->getPlatformMapping(),
        getHwSwitch()->getPlatform()->getAsic(),
        {masterLogicalPortIds()[0], masterLogicalPortIds()[1]},
        getHwSwitch()->getPlatform()->supportsAddRemovePort());
  }

  cfg::SwitchConfig setupDefaultQueueWithPfcMaps() {
    auto config = initialConfig();
    cfg::QosMap qosMap;
    std::map<int16_t, int16_t> tc2PgId;
    std::map<int16_t, int16_t> pfcPri2PgId;
    std::map<int16_t, int16_t> pfcPri2QueueId;

    for (auto i = 0; i < kTrafficClassToPgId.size(); i++) {
      tc2PgId.emplace(i, kTrafficClassToPgId[i]);
    }
    for (auto i = 0; i < kPfcPriorityToPgId.size(); i++) {
      pfcPri2PgId.emplace(i, kPfcPriorityToPgId[i]);
    }

    for (auto i = 0; i < kPfcPriorityToQueue.size(); ++i) {
      pfcPri2QueueId.emplace(i, kPfcPriorityToQueue[i]);
    }

    // setup pfc maps
    qosMap.trafficClassToPgId() = tc2PgId;
    qosMap.pfcPriorityToPgId() = pfcPri2PgId;
    qosMap.pfcPriorityToQueueId() = pfcPri2QueueId;

    config.qosPolicies()->resize(1);
    config.qosPolicies()[0].name() = "qp";
    config.qosPolicies()[0].qosMap() = qosMap;

    cfg::TrafficPolicyConfig dataPlaneTrafficPolicy;
    dataPlaneTrafficPolicy.defaultQosPolicy() = "qp";
    config.dataPlaneTrafficPolicy() = dataPlaneTrafficPolicy;
    applyNewConfig(config);
    return config;
  }

  void validateTc2PgId(const std::vector<int>& expectedTc2Pg) {
    auto tc2PgId = BcmQosPolicy::readPriorityGroupMapping(
        getHwSwitch(),
        bcmCosqInputPriPriorityGroupMcMapping,
        "bcmCosqInputPriPriorityGroupMcMapping");
    // all entries should be same
    EXPECT_TRUE(
        std::equal(
            expectedTc2Pg.begin(),
            expectedTc2Pg.end(),
            tc2PgId.begin(),
            tc2PgId.end()));
  }

  void validatePfcPri2PgId(const std::vector<int>& expectedPfcPri2Pg) {
    auto pfcPri2PgId = BcmQosPolicy::readPfcPriorityToPg(getHwSwitch());
    EXPECT_EQ(pfcPri2PgId.size(), expectedPfcPri2Pg.size());
    // all entries should be same
    EXPECT_TRUE(
        std::equal(
            expectedPfcPri2Pg.begin(),
            expectedPfcPri2Pg.end(),
            pfcPri2PgId.begin(),
            pfcPri2PgId.end()));
  }

  const std::vector<bcm_cosq_pfc_class_map_config_t>
  convertPfcPriToCosqPfcClassMapStruct(
      const std::vector<int>& pfcPri2QueueId,
      const std::vector<int>& pfcEnable,
      const std::vector<int>& pfcOptimized) {
    EXPECT_EQ(pfcOptimized.size(), pfcEnable.size());
    auto pfcClassQueue = BcmQosPolicy::getBcmHwDefaultsPfcPriToQueueMapping();
    for (auto i = 0; i < pfcPri2QueueId.size(); ++i) {
      pfcClassQueue[i].pfc_enable = pfcEnable[i];
      pfcClassQueue[i].pfc_optimized = pfcOptimized[i];
      pfcClassQueue[i].cos_list_bmp = (1 << pfcPri2QueueId[i]);
    }
    return pfcClassQueue;
  }

  void validatePfcPri2QueueId(
      const std::vector<bcm_cosq_pfc_class_map_config_t>&
          expectedPfcPri2Queue) {
    auto pfcPri2QueueId = BcmQosPolicy::readPfcPriorityToQueue(getHwSwitch());
    EXPECT_EQ(pfcPri2QueueId.size(), expectedPfcPri2Queue.size());
    auto pfcPri2QueueIdSize = pfcPri2QueueId.size();
    for (int i = 0; i < pfcPri2QueueIdSize; ++i) {
      EXPECT_EQ(
          pfcPri2QueueId[i].cos_list_bmp, expectedPfcPri2Queue[i].cos_list_bmp);
    }
  }

  void validateAllPfcMaps(
      const std::vector<int>& tc2Pg,
      const std::vector<int>& pfcPri2Pg,
      const std::vector<int>& pfcPri2Queue) {
    if (!isSupported(HwAsic::Feature::PFC)) {
      return;
    }
    validateTc2PgId(tc2Pg);
    validatePfcPri2PgId(pfcPri2Pg);
    // currently there is no interface exposed to program
    // "PfcEnable" and "PfcOptimize" ..hence use the defaults here
    const auto& pfcClassMapping = convertPfcPriToCosqPfcClassMapStruct(
        pfcPri2Queue,
        getDefaultPfcEnablePriorityToQueue(),
        getDefaultPfcOptimizedPriorityToQueue());
    validatePfcPri2QueueId(pfcClassMapping);
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

    config.qosPolicies()->resize(1);
    *config.qosPolicies()[0].name() = "qp";
    config.qosPolicies()[0].rules()->resize(8);
    for (auto i = 0; i < 8; i++) {
      config.qosPolicies()[0].rules()[i].dscp()->resize(8);
      for (auto j = 0; j < 8; j++) {
        config.qosPolicies()[0].rules()[i].dscp()[j] = 8 * i + j;
      }
    }
    cfg::TrafficPolicyConfig dataPlaneTrafficPolicy;
    dataPlaneTrafficPolicy.defaultQosPolicy() = "qp";
    config.dataPlaneTrafficPolicy() = dataPlaneTrafficPolicy;
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

// configure trafficClassToPg map, remove the qos
// policy so that trafficClassToPg map is reset  to default
// Query HW to validate the same
TEST_F(BcmQosMapTest, PfcMapsRemovePolicy) {
  auto setup = [this]() {
    auto config = setupDefaultQueueWithPfcMaps();
    // reset qosPolicy
    config = initialConfig();
    applyNewConfig(config);
  };

  auto verify = [this]() {
    validateAllPfcMaps(
        getDefaultTrafficClassToPg(), getDefaultPfcPriorityToPg(), {});
  };
  verifyAcrossWarmBoots(setup, verify);
}

// configure trafficClassToPg map and remove it
// explicitly, so that defaults get programmed
// Query HW to validate the same.
// Since we reset the trafficClassToPg map explicitly
// it takes a diffeent code path from Tc2PgRemovePolicy
TEST_F(BcmQosMapTest, PfcMapsReset) {
  auto setup = [this]() {
    cfg::QosMap qosMap;
    auto config = setupDefaultQueueWithPfcMaps();
    // reset TC <-> PG Id
    qosMap.trafficClassToPgId().reset();
    qosMap.pfcPriorityToPgId().reset();
    config.qosPolicies()[0].qosMap() = qosMap;
    applyNewConfig(config);
  };

  auto verify = [this]() {
    validateAllPfcMaps(
        getDefaultTrafficClassToPg(), getDefaultPfcPriorityToPg(), {});
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmQosMapTest, BcmAllQosMapsWithPfcMaps) {
  auto setup = [this]() {
    auto config = initialConfig();

    cfg::QosMap qosMap;
    qosMap.dscpMaps()->resize(8);
    for (auto i = 0; i < 8; i++) {
      qosMap.dscpMaps()[i].internalTrafficClass() = i;
      for (auto j = 0; j < 8; j++) {
        qosMap.dscpMaps()[i].fromDscpToTrafficClass()->push_back(8 * i + j);
      }
    }
    qosMap.expMaps()->resize(8);
    for (auto i = 0; i < 8; i++) {
      qosMap.expMaps()[i].internalTrafficClass() = i;
      qosMap.expMaps()[i].fromExpToTrafficClass()->push_back(i);
      qosMap.expMaps()[i].fromTrafficClassToExp() = i;
    }

    std::map<int16_t, int16_t> tc2PgId;
    std::map<int16_t, int16_t> pfcPri2PgId;
    std::map<int16_t, int16_t> pfcPri2QueueId;

    for (auto i = 0; i < kTrafficClassToPgId.size(); i++) {
      // add trafficClassToPgId mappings as well
      tc2PgId.emplace(i, kTrafficClassToPgId[i]);
    }

    for (auto i = 0; i < kPfcPriorityToPgId.size(); ++i) {
      // add pfc priority to PG id mappings
      pfcPri2PgId.emplace(i, kPfcPriorityToPgId[i]);
    }

    for (auto i = 0; i < kPfcPriorityToQueue.size(); ++i) {
      pfcPri2QueueId.emplace(i, kPfcPriorityToQueue[i]);
    }

    qosMap.trafficClassToPgId() = tc2PgId;
    qosMap.pfcPriorityToPgId() = pfcPri2PgId;
    qosMap.pfcPriorityToQueueId() = pfcPri2QueueId;

    config.qosPolicies()->resize(1);
    config.qosPolicies()[0].name() = "qp";
    config.qosPolicies()[0].qosMap() = qosMap;

    cfg::TrafficPolicyConfig dataPlaneTrafficPolicy;
    dataPlaneTrafficPolicy.defaultQosPolicy() = "qp";
    config.dataPlaneTrafficPolicy() = dataPlaneTrafficPolicy;
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
        // TH4 always return 48 entries not including ghost ones when
        // qos_map_multi_get_mode is 1.
        EXPECT_TRUE(array_count == 64 || array_count == 48);
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

    validateAllPfcMaps(
        kTrafficClassToPgIdInHw, kPfcPriorityToPgId, kPfcPriorityToQueue);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmQosMapTest, BcmAllQosMaps) {
  auto setup = [this]() {
    auto config = initialConfig();

    cfg::QosMap qosMap;
    qosMap.dscpMaps()->resize(8);
    for (auto i = 0; i < 8; i++) {
      *qosMap.dscpMaps()[i].internalTrafficClass() = i;
      for (auto j = 0; j < 8; j++) {
        qosMap.dscpMaps()[i].fromDscpToTrafficClass()->push_back(8 * i + j);
      }
    }
    qosMap.expMaps()->resize(8);
    for (auto i = 0; i < 8; i++) {
      *qosMap.expMaps()[i].internalTrafficClass() = i;
      qosMap.expMaps()[i].fromExpToTrafficClass()->push_back(i);
      qosMap.expMaps()[i].fromTrafficClassToExp() = i;
    }

    for (auto i = 0; i < 8; i++) {
      qosMap.trafficClassToQueueId()->emplace(i, i);
    }
    config.qosPolicies()->resize(1);
    *config.qosPolicies()[0].name() = "qp";
    config.qosPolicies()[0].qosMap() = qosMap;

    cfg::TrafficPolicyConfig dataPlaneTrafficPolicy;
    dataPlaneTrafficPolicy.defaultQosPolicy() = "qp";
    config.dataPlaneTrafficPolicy() = dataPlaneTrafficPolicy;
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
        // TH4 always return 48 entries not including ghost ones when
        // qos_map_multi_get_mode is 1.
        EXPECT_TRUE(array_count == 64 || array_count == 48);
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
    validateAllPfcMaps(
        getDefaultTrafficClassToPg(), getDefaultPfcPriorityToPg(), {});
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
