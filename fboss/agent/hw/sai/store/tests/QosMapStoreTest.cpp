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
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

#include <variant>

using namespace facebook::fboss;

class QosMapStoreTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    saiApiTable = SaiApiTable::getInstance();
    saiApiTable->queryApis();
  }
  std::shared_ptr<FakeSai> fs;
  std::shared_ptr<SaiApiTable> saiApiTable;

  QosMapSaiId createQosMap(int type) {
    std::vector<sai_qos_map_t> mapping;
    SaiQosMapTraits::CreateAttributes c{type, mapping};
    return saiApiTable->qosMapApi().create<SaiQosMapTraits>(c, 0);
  }
};

TEST_F(QosMapStoreTest, loadQosMaps) {
  auto qosMapSaiId1 = createQosMap(SAI_QOS_MAP_TYPE_DSCP_TO_TC);
  auto qosMapSaiId2 = createQosMap(SAI_QOS_MAP_TYPE_TC_TO_QUEUE);

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiQosMapTraits>();

  SaiQosMapTraits::AdapterHostKey k1{SAI_QOS_MAP_TYPE_DSCP_TO_TC};
  SaiQosMapTraits::AdapterHostKey k2{SAI_QOS_MAP_TYPE_TC_TO_QUEUE};
  auto got = store.get(k1);
  EXPECT_EQ(got->adapterKey(), qosMapSaiId1);
  got = store.get(k2);
  EXPECT_EQ(got->adapterKey(), qosMapSaiId2);
}

TEST_F(QosMapStoreTest, qosMapLoadCtor) {
  auto qosMapSaiId = createQosMap(SAI_QOS_MAP_TYPE_DSCP_TO_TC);

  SaiObject<SaiQosMapTraits> obj(qosMapSaiId);
  EXPECT_EQ(obj.adapterKey(), qosMapSaiId);
  EXPECT_EQ(
      GET_ATTR(QosMap, Type, obj.attributes()), SAI_QOS_MAP_TYPE_DSCP_TO_TC);
}

TEST_F(QosMapStoreTest, qosMapCreateCtor) {
  std::vector<sai_qos_map_t> mapping;
  SaiQosMapTraits::AdapterHostKey k{SAI_QOS_MAP_TYPE_DSCP_TO_TC};
  SaiQosMapTraits::CreateAttributes c{SAI_QOS_MAP_TYPE_DSCP_TO_TC, mapping};
  SaiObject<SaiQosMapTraits> obj(k, c, 0);
  EXPECT_EQ(
      GET_ATTR(QosMap, Type, obj.attributes()), SAI_QOS_MAP_TYPE_DSCP_TO_TC);
}

TEST_F(QosMapStoreTest, qosMapSetMapping) {
  std::vector<sai_qos_map_t> mapping;
  SaiQosMapTraits::AdapterHostKey k{SAI_QOS_MAP_TYPE_DSCP_TO_TC};
  SaiQosMapTraits::CreateAttributes c{SAI_QOS_MAP_TYPE_DSCP_TO_TC, mapping};
  SaiObject<SaiQosMapTraits> obj(k, c, 0);
  EXPECT_EQ(
      GET_ATTR(QosMap, Type, obj.attributes()), SAI_QOS_MAP_TYPE_DSCP_TO_TC);
  auto gotMapping = GET_ATTR(QosMap, MapToValueList, obj.attributes());
  EXPECT_EQ(gotMapping.size(), 0);

  mapping.resize(1);
  mapping[0].key.dscp = 2;
  mapping[0].value.tc = 4;
  SaiQosMapTraits::CreateAttributes newAttrs{SAI_QOS_MAP_TYPE_DSCP_TO_TC,
                                             mapping};
  obj.setAttributes(newAttrs);

  EXPECT_EQ(
      GET_ATTR(QosMap, Type, obj.attributes()), SAI_QOS_MAP_TYPE_DSCP_TO_TC);
  gotMapping = GET_ATTR(QosMap, MapToValueList, obj.attributes());
  EXPECT_EQ(gotMapping.size(), 1);
  EXPECT_EQ(gotMapping[0].key.dscp, 2);
  EXPECT_EQ(gotMapping[0].value.tc, 4);
}

TEST_F(QosMapStoreTest, serDeser) {
  auto id = createQosMap(SAI_QOS_MAP_TYPE_DSCP_TO_TC);
  verifyAdapterKeySerDeser<SaiQosMapTraits>({id});
}

TEST_F(QosMapStoreTest, toStr) {
  std::ignore = createQosMap(SAI_QOS_MAP_TYPE_DSCP_TO_TC);
  verifyToStr<SaiQosMapTraits>();
}
