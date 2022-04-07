/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/CounterApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"

#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
class CounterStoreTest : public SaiStoreTest {
 public:
  SaiCounterTraits::CreateAttributes counterCreateAtts() const {
    SaiCharArray32 label = {"testCounter"};
    return SaiCounterTraits::CreateAttributes{label, SAI_COUNTER_TYPE_REGULAR};
  }
  CounterSaiId createCounter() {
    return saiApiTable->counterApi().create<SaiCounterTraits>(
        counterCreateAtts(), 0);
  }
};

TEST_F(CounterStoreTest, loadCounter) {
  auto id = createCounter();

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiCounterTraits>();

  SaiCounterTraits::AdapterHostKey k{counterCreateAtts()};
  auto got = store.get(k);
  EXPECT_NE(got, nullptr);
  EXPECT_EQ(got->adapterKey(), id);
}

TEST_F(CounterStoreTest, counterLoadCtor) {
  auto counterId = createCounter();
  auto counterObj = createObj<SaiCounterTraits>(counterId);
  EXPECT_EQ(counterObj.adapterKey(), counterId);
  EXPECT_EQ(
      GET_ATTR(Counter, Type, counterObj.attributes()),
      SAI_COUNTER_TYPE_REGULAR);
  auto labelValueGot = GET_ATTR(Counter, Label, counterObj.attributes());
  SaiCharArray32 labelValueExpected = {"testCounter"};
  EXPECT_EQ(labelValueExpected, labelValueGot);
}

TEST_F(CounterStoreTest, counterCreateCtor) {
  auto attrs = counterCreateAtts();
  SaiCounterTraits::AdapterHostKey adapterHostKey = attrs;
  auto obj = createObj<SaiCounterTraits>(adapterHostKey, attrs, 0);
  auto counterType = saiApiTable->counterApi().getAttribute(
      obj.adapterKey(), SaiCounterTraits::Attributes::Type{});
  EXPECT_EQ(counterType, SAI_COUNTER_TYPE_REGULAR);
  auto labelValueGot = GET_ATTR(Counter, Label, obj.attributes());
  SaiCharArray32 labelValueExpected = {"testCounter"};
  EXPECT_EQ(labelValueExpected, labelValueGot);
}

TEST_F(CounterStoreTest, serDeser) {
  auto id = createCounter();
  verifyAdapterKeySerDeser<SaiCounterTraits>({id});
}

TEST_F(CounterStoreTest, toStr) {
  std::ignore = createCounter();
  verifyToStr<SaiCounterTraits>();
}
#endif
