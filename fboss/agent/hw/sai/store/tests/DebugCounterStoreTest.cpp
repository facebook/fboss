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
                {SAI_IN_DROP_REASON_BLACKHOLE_ROUTE}},
            SaiDebugCounterTraits::Attributes::OutDropReasons{}};
  }
  DebugCounterSaiId createInDebugCounter() {
    return saiApiTable->debugCounterApi().create<SaiDebugCounterTraits>(
        inCounterCreateAtts(), 0);
  }
  SaiDebugCounterTraits::CreateAttributes outCounterCreateAtts() const {
    return {SAI_DEBUG_COUNTER_TYPE_PORT_OUT_DROP_REASONS,
            SAI_DEBUG_COUNTER_BIND_METHOD_AUTOMATIC,
            SaiDebugCounterTraits::Attributes::InDropReasons{},
            SaiDebugCounterTraits::Attributes::OutDropReasons{
                {SAI_OUT_DROP_REASON_EGRESS_VLAN_FILTER}}};
  }
  DebugCounterSaiId createOutDebugCounter() {
    return saiApiTable->debugCounterApi().create<SaiDebugCounterTraits>(
        outCounterCreateAtts(), 0);
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

TEST_F(DebugCounterStoreTest, loadOutDebugCounter) {
  auto id = createOutDebugCounter();

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiDebugCounterTraits>();

  SaiDebugCounterTraits::AdapterHostKey k{outCounterCreateAtts()};
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
  auto attrs = outCounterCreateAtts();
  SaiDebugCounterTraits::AdapterHostKey adapterHostKey = attrs;
  SaiObject<SaiDebugCounterTraits> obj(adapterHostKey, attrs, 0);
  auto outDropReasons = saiApiTable->debugCounterApi().getAttribute(
      obj.adapterKey(), SaiDebugCounterTraits::Attributes::OutDropReasons{});
  EXPECT_EQ(
      outDropReasons,
      SaiDebugCounterTraits::Attributes::OutDropReasons{
          {SAI_OUT_DROP_REASON_EGRESS_VLAN_FILTER}}
          .value());
}

TEST_F(DebugCounterStoreTest, serDeser) {
  auto id = createInDebugCounter();
  auto id2 = createOutDebugCounter();
  verifyAdapterKeySerDeser<SaiDebugCounterTraits>({id, id2});
}
