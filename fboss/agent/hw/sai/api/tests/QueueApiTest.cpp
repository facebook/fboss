/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/QueueApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class QueueApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    queueApi = std::make_unique<QueueApi>();
  }
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<QueueApi> queueApi;

  QueueSaiId
  createQueue(PortSaiId saiPortId, bool unicast, uint8_t queueIndex) {
    SaiQueueTraits::Attributes::Type type(
        unicast ? SAI_QUEUE_TYPE_UNICAST : SAI_QUEUE_TYPE_MULTICAST);
    SaiQueueTraits::Attributes::Port port(saiPortId);
    SaiQueueTraits::Attributes::Index queueId(queueIndex);
    SaiQueueTraits::Attributes::ParentSchedulerNode schedulerNode(saiPortId);
    SaiQueueTraits::CreateAttributes a{type, port, queueId, schedulerNode};
    auto saiQueueId = queueApi->create<SaiQueueTraits>(a, 0);
    return saiQueueId;
  }

  void checkQueue(QueueSaiId queueId) {
    SaiQueueTraits::Attributes::Type typeAttribute;
    auto gotType = queueApi->getAttribute(queueId, typeAttribute);
    SaiQueueTraits::Attributes::Port portAttribute;
    auto gotPort = queueApi->getAttribute(queueId, portAttribute);
    SaiQueueTraits::Attributes::Index indexAttribute;
    auto gotIndex = queueApi->getAttribute(queueId, indexAttribute);
    SaiQueueTraits::Attributes::ParentSchedulerNode schedulerAttribute;
    auto gotScheduler = queueApi->getAttribute(queueId, schedulerAttribute);
    EXPECT_EQ(fs->qm.get(queueId).type, gotType);
    EXPECT_EQ(fs->qm.get(queueId).port, gotPort);
    EXPECT_EQ(fs->qm.get(queueId).index, gotIndex);
    EXPECT_EQ(fs->qm.get(queueId).parentScheduler, gotScheduler);
  }
};

TEST_F(QueueApiTest, createQueue) {
  PortSaiId saiPortId{1};
  uint8_t queueIndex = 4;
  auto saiQueueId = createQueue(saiPortId, true, queueIndex);
  checkQueue(saiQueueId);
  queueApi->remove(saiQueueId);
}

TEST_F(QueueApiTest, removeQueue) {
  PortSaiId saiPortId{1};
  uint8_t queueIndex = 10;
  auto saiQueueId = createQueue(saiPortId, true, queueIndex);
  checkQueue(saiQueueId);
  EXPECT_EQ(fs->qm.map().size(), 1);
  queueApi->remove(saiQueueId);
  EXPECT_EQ(fs->qm.map().size(), 0);
}
