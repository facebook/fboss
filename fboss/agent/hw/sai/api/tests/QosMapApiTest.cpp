/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/api/QosMapApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class QosMapApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    qosMapApi = std::make_unique<QosMapApi>();
  }
  QosMapSaiId createDscpToTcQosMap(
      const std::unordered_map<int, int>& dscpToTc) {
    SaiQosMapTraits::Attributes::Type typeAttribute(
        SAI_QOS_MAP_TYPE_DSCP_TO_TC);
    std::vector<sai_qos_map_t> qosMapList;
    qosMapList.resize(dscpToTc.size());
    int i = 0;
    for (const auto& mapping : dscpToTc) {
      qosMapList[i].key.dscp = mapping.first;
      qosMapList[i].value.tc = mapping.second;
      ++i;
    }
    SaiQosMapTraits::Attributes::MapToValueList mapToValueListAttribute(
        qosMapList);
    auto qosMapId = qosMapApi->create<SaiQosMapTraits>(
        {typeAttribute, mapToValueListAttribute}, 0);
    EXPECT_EQ(qosMapId, fs->qosMapManager.get(qosMapId).id);
    return qosMapId;
  }

  void checkDscpToTcQosMap(
      QosMapSaiId qosMapSaiId,
      const std::unordered_map<int, int>& dscpToTc) {
    const auto& qm = fs->qosMapManager.get(qosMapSaiId);
    for (const auto& mapping : qm.mapToValueList) {
      auto expectedItr = dscpToTc.find(mapping.key.dscp);
      EXPECT_NE(expectedItr, dscpToTc.end());
      EXPECT_EQ(expectedItr->second, mapping.value.tc);
    }
  }

  void testDscpToTcQosMap(const std::unordered_map<int, int>& dscpToTc) {
    auto id = createDscpToTcQosMap(dscpToTc);
    checkDscpToTcQosMap(id, dscpToTc);
  }

  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<QosMapApi> qosMapApi;
  sai_object_id_t switchId;
};

TEST_F(QosMapApiTest, create) {
  std::unordered_map<int, int> dscpToTc{
      {0, 0}, {1, 0}, {5, 2}, {2, 1}, {3, 7}, {4, 1}};
  testDscpToTcQosMap(dscpToTc);
}

TEST_F(QosMapApiTest, remove) {
  std::unordered_map<int, int> dscpToTc{
      {0, 0}, {1, 0}, {5, 2}, {2, 1}, {3, 7}, {4, 1}};
  auto id = createDscpToTcQosMap(dscpToTc);
  qosMapApi->remove(id);
  EXPECT_EQ(fs->qosMapManager.map().size(), 0);
}

TEST_F(QosMapApiTest, sai_qos_map_t_Equality) {
  sai_qos_map_t qm1{};
  qm1.key.dscp = 3;
  qm1.key.tc = 4;
  sai_qos_map_t qm2{};
  qm2.key.dscp = 3;
  qm2.key.tc = 4;
  EXPECT_EQ(qm1, qm2);
  qm2.key.tc = 5;
  EXPECT_NE(qm1, qm2);
  qm2.key.tc = 4;
  qm1.value.color = SAI_PACKET_COLOR_RED;
  EXPECT_NE(qm1, qm2);
}

TEST_F(QosMapApiTest, getQosMapAttribute) {
  std::unordered_map<int, int> dscpToTc{
      {0, 0}, {1, 0}, {5, 2}, {2, 1}, {3, 7}, {4, 1}};
  auto qosMapId = createDscpToTcQosMap(dscpToTc);

  auto qosMapTypeGot =
      qosMapApi->getAttribute(qosMapId, SaiQosMapTraits::Attributes::Type());
  auto qosMapToValueListGot = qosMapApi->getAttribute(
      qosMapId, SaiQosMapTraits::Attributes::MapToValueList());

  EXPECT_EQ(qosMapTypeGot, SAI_QOS_MAP_TYPE_DSCP_TO_TC);
  EXPECT_EQ(qosMapToValueListGot.size(), dscpToTc.size());
}

TEST_F(QosMapApiTest, setQosMapAttribute) {
  std::unordered_map<int, int> dscpToTc{
      {0, 0}, {1, 0}, {5, 2}, {2, 1}, {3, 7}, {4, 1}};
  auto qosMapId = createDscpToTcQosMap(dscpToTc);

  // QosMap supports setting MapToValueList post creation
  std::vector<sai_qos_map_t> qosMapList{};
  SaiQosMapTraits::Attributes::MapToValueList qosMapToValueList{qosMapList};
  qosMapApi->setAttribute(qosMapId, qosMapToValueList);

  EXPECT_EQ(
      qosMapApi
          ->getAttribute(
              qosMapId, SaiQosMapTraits::Attributes::MapToValueList())
          .size(),
      qosMapList.size());

  // QosMap does not support setting type attribute post creation
  SaiQosMapTraits::Attributes::Type qosMapType(SAI_QOS_MAP_TYPE_DSCP_TO_TC);

  EXPECT_THROW(qosMapApi->setAttribute(qosMapId, qosMapType), SaiApiError);
}
