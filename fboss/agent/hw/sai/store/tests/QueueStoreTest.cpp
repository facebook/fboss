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
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"

#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

class QueueStoreTest : public SaiStoreTest {
 public:
  QueueSaiId
  createQueue(sai_queue_type_t type, sai_object_id_t portId, uint8_t queueId) {
    SaiQueueTraits::CreateAttributes c =
        SaiQueueTraits::CreateAttributes{type,
                                         portId,
                                         queueId,
                                         portId,
                                         std::nullopt,
                                         std::nullopt,
                                         std::nullopt};
    return saiApiTable->queueApi().create<SaiQueueTraits>(c, 0);
  }
};

TEST_F(QueueStoreTest, loadQueue) {
  // create a queue
  auto id = createQueue(SAI_QUEUE_TYPE_MULTICAST, 1, 4);
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiQueueTraits>();
  SaiQueueTraits::AdapterHostKey k{SAI_QUEUE_TYPE_MULTICAST, 1, 4};
  auto got = store.get(k);
  EXPECT_EQ(got->adapterKey(), id);
}

TEST_F(QueueStoreTest, queueLoadCtor) {
  auto id = createQueue(SAI_QUEUE_TYPE_MULTICAST, 1, 4);
  SaiObject<SaiQueueTraits> obj(id);
  EXPECT_EQ(obj.adapterKey(), id);
  EXPECT_EQ(GET_ATTR(Queue, Type, obj.attributes()), SAI_QUEUE_TYPE_MULTICAST);
  EXPECT_EQ(GET_ATTR(Queue, Port, obj.attributes()), 1);
  EXPECT_EQ(GET_ATTR(Queue, Index, obj.attributes()), 4);
}

TEST_F(QueueStoreTest, queueCreateCtor) {
  SaiQueueTraits::AdapterHostKey k{SAI_QUEUE_TYPE_UNICAST, 2, 6};
  SaiQueueTraits::CreateAttributes c =
      SaiQueueTraits::CreateAttributes{SAI_QUEUE_TYPE_UNICAST,
                                       2,
                                       6,
                                       2,
                                       std::nullopt,
                                       std::nullopt,
                                       std::nullopt};
  SaiObject<SaiQueueTraits> obj(k, c, 0);
  EXPECT_EQ(GET_ATTR(Queue, Type, obj.attributes()), SAI_QUEUE_TYPE_UNICAST);
  EXPECT_EQ(GET_ATTR(Queue, Port, obj.attributes()), 2);
  EXPECT_EQ(GET_ATTR(Queue, Index, obj.attributes()), 6);
}

TEST_F(QueueStoreTest, serDeser) {
  // create a queue
  auto id = createQueue(SAI_QUEUE_TYPE_UNICAST, 1, 4);
  // Queues 0-7 are created by FakeSai to mimic ASIC coming up with queues
  // already created
  // Look for this queue key in queue adaptor JSON
  verifyAdapterKeySerDeser<SaiQueueTraits>({id}, true);
}

TEST_F(QueueStoreTest, toStr) {
  std::ignore = createQueue(SAI_QUEUE_TYPE_UNICAST, 1, 4);
  verifyToStr<SaiQueueTraits>();
}
