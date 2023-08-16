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
    SaiQueueTraits::CreateAttributes a{
        type, port, queueId, schedulerNode, 42, 24, 4242};
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
    SaiQueueTraits::Attributes::SchedulerProfileId schedulerId;
    auto gotSchedulerId = queueApi->getAttribute(queueId, schedulerId);
    SaiQueueTraits::Attributes::WredProfileId wredId;
    auto gotWredId = queueApi->getAttribute(queueId, wredId);
    SaiQueueTraits::Attributes::BufferProfileId bufferId;
    auto gotBufferId = queueApi->getAttribute(queueId, bufferId);
    SaiQueueTraits::Attributes::EnablePfcDldr enablePfcDldr;
    auto gotEnablePfcDldr = queueApi->getAttribute(queueId, enablePfcDldr);
    EXPECT_EQ(fs->queueManager.get(queueId).type, gotType);
    EXPECT_EQ(fs->queueManager.get(queueId).port, gotPort);
    EXPECT_EQ(fs->queueManager.get(queueId).index, gotIndex);
    EXPECT_EQ(fs->queueManager.get(queueId).parentScheduler, gotScheduler);
    EXPECT_EQ(fs->queueManager.get(queueId).schedulerProfileId, gotSchedulerId);
    EXPECT_EQ(fs->queueManager.get(queueId).wredProfileId, gotWredId);
    EXPECT_EQ(fs->queueManager.get(queueId).bufferProfileId, gotBufferId);
    EXPECT_EQ(fs->queueManager.get(queueId).enablePfcDldr, gotEnablePfcDldr);
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
  uint8_t numCpuQueues = 8;
  auto saiQueueId = createQueue(saiPortId, true, queueIndex);
  checkQueue(saiQueueId);
  EXPECT_EQ(fs->queueManager.map().size(), 1 + numCpuQueues);
  queueApi->remove(saiQueueId);
  EXPECT_EQ(fs->queueManager.map().size(), numCpuQueues);
}

TEST_F(QueueApiTest, setQueueAttribute) {
  PortSaiId saiPortId{1};
  uint8_t queueIndex = 10;
  auto saiQueueId = createQueue(saiPortId, true, queueIndex);
  checkQueue(saiQueueId);

  SaiQueueTraits::Attributes::ParentSchedulerNode parentSchedulerNode{2};
  SaiQueueTraits::Attributes::WredProfileId wredProfileId{1};
  SaiQueueTraits::Attributes::BufferProfileId bufferProfileId{1};
  SaiQueueTraits::Attributes::SchedulerProfileId schedulerProfileId{1};
  SaiQueueTraits::Attributes::WredProfileId wredrProfileId{2};
  SaiQueueTraits::Attributes::EnablePfcDldr enablePfcDldr{true};

  queueApi->setAttribute(saiQueueId, parentSchedulerNode);
  queueApi->setAttribute(saiQueueId, wredProfileId);
  queueApi->setAttribute(saiQueueId, bufferProfileId);
  queueApi->setAttribute(saiQueueId, schedulerProfileId);
  queueApi->setAttribute(saiQueueId, enablePfcDldr);

  SaiQueueTraits::Attributes::ParentSchedulerNode parentSchedulerNodeGot =
      queueApi->getAttribute(
          saiQueueId, SaiQueueTraits::Attributes::ParentSchedulerNode());
  SaiQueueTraits::Attributes::WredProfileId wredProfileIdGot =
      queueApi->getAttribute(
          saiQueueId, SaiQueueTraits::Attributes::WredProfileId());
  SaiQueueTraits::Attributes::BufferProfileId bufferProfileIdGot =
      queueApi->getAttribute(
          saiQueueId, SaiQueueTraits::Attributes::BufferProfileId());
  SaiQueueTraits::Attributes::SchedulerProfileId schedulerProfileIdGot =
      queueApi->getAttribute(
          saiQueueId, SaiQueueTraits::Attributes::SchedulerProfileId());
  SaiQueueTraits::Attributes::EnablePfcDldr enablePfcDldrGot =
      queueApi->getAttribute(
          saiQueueId, SaiQueueTraits::Attributes::EnablePfcDldr());

  EXPECT_EQ(parentSchedulerNodeGot, parentSchedulerNode);
  EXPECT_EQ(wredProfileIdGot, wredProfileId);
  EXPECT_EQ(bufferProfileIdGot, bufferProfileId);
  EXPECT_EQ(schedulerProfileIdGot, schedulerProfileId);
  EXPECT_EQ(wredProfileIdGot, wredProfileId);
  EXPECT_EQ(enablePfcDldrGot, enablePfcDldr);

  // Queue does not support setting type, port and index attributes post
  // creation

  SaiQueueTraits::Attributes::Type type{SAI_QUEUE_TYPE_MULTICAST};
  SaiQueueTraits::Attributes::Port port{saiPortId};
  SaiQueueTraits::Attributes::Index queueId{queueIndex};

  EXPECT_THROW(queueApi->setAttribute(saiQueueId, type), SaiApiError);
  EXPECT_THROW(queueApi->setAttribute(saiQueueId, port), SaiApiError);
  EXPECT_THROW(queueApi->setAttribute(saiQueueId, queueId), SaiApiError);
}

TEST_F(QueueApiTest, formatQueueAttributes) {
  SaiQueueTraits::Attributes::Type t{SAI_QUEUE_TYPE_UNICAST};
  EXPECT_EQ("Type: 1", fmt::format("{}", t));
  SaiQueueTraits::Attributes::Port p{42};
  EXPECT_EQ("Port: 42", fmt::format("{}", p));
  SaiQueueTraits::Attributes::Index i{3};
  EXPECT_EQ("Index: 3", fmt::format("{}", i));
  SaiQueueTraits::Attributes::ParentSchedulerNode psn{42};
  EXPECT_EQ("ParentSchedulerNode: 42", fmt::format("{}", psn));
  SaiQueueTraits::Attributes::SchedulerProfileId schedulerProfileId{24};
  EXPECT_EQ("SchedulerProfileId: 24", fmt::format("{}", schedulerProfileId));
  SaiQueueTraits::Attributes::WredProfileId wredProfileId{24};
  EXPECT_EQ("WredProfileId: 24", fmt::format("{}", wredProfileId));
}
