/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/DebugCounterApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"

#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

class DebugCounterStoreTest : public SaiStoreTest {
 public:
  SaiDebugCounterTraits::CreateAttributes inCounterCreateAtts() const {
    return {SAI_DEBUG_COUNTER_TYPE_PORT_IN_DROP_REASONS,
            SAI_DEBUG_COUNTER_BIND_METHOD_AUTOMATIC,
            SaiDebugCounterTraits::Attributes::InDropReasons{
                {SAI_IN_DROP_REASON_BLACKHOLE_ROUTE}}};
  }
  DebugCounterSaiId createInDebugCounter() {
    return saiApiTable->debugCounterApi().create<SaiDebugCounterTraits>(
        inCounterCreateAtts(), 0);
  }
};

TEST_F(DebugCounterStoreTest, loadInDebugCounter) {
  auto id = createInDebugCounter();

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiDebugCounterTraits>();

  SaiDebugCounterTraits::AdapterHostKey k{inCounterCreateAtts()};
  auto got = store.get(k);
  EXPECT_NE(got, nullptr);
  EXPECT_EQ(got->adapterKey(), id);
}

TEST_F(DebugCounterStoreTest, debugCounterLoadCtor) {
  auto debugCounterId = createInDebugCounter();
  SaiObject<SaiDebugCounterTraits> debugCounterObj(debugCounterId);
  EXPECT_EQ(debugCounterObj.adapterKey(), debugCounterId);
  EXPECT_EQ(
      GET_OPT_ATTR(DebugCounter, InDropReasons, debugCounterObj.attributes()),
      SaiDebugCounterTraits::Attributes::InDropReasons{
          {SAI_IN_DROP_REASON_BLACKHOLE_ROUTE}}
          .value());
}

TEST_F(DebugCounterStoreTest, debugCounterCreateCtor) {
  auto attrs = inCounterCreateAtts();
  SaiDebugCounterTraits::AdapterHostKey adapterHostKey = attrs;
  SaiObject<SaiDebugCounterTraits> obj(adapterHostKey, attrs, 0);
  auto outDropReasons = saiApiTable->debugCounterApi().getAttribute(
      obj.adapterKey(), SaiDebugCounterTraits::Attributes::InDropReasons{});
  EXPECT_EQ(
      outDropReasons,
      SaiDebugCounterTraits::Attributes::InDropReasons{
          {SAI_IN_DROP_REASON_BLACKHOLE_ROUTE}}
          .value());
}

TEST_F(DebugCounterStoreTest, serDeser) {
  auto id = createInDebugCounter();
  verifyAdapterKeySerDeser<SaiDebugCounterTraits>({id});
}

TEST_F(DebugCounterStoreTest, toStr) {
  std::ignore = createInDebugCounter();
  verifyToStr<SaiDebugCounterTraits>();
}
