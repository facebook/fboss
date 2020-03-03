/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"

#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

class PortStoreTest : public SaiStoreTest {
 public:
  SaiPortTraits::CreateAttributes makeAttrs(
      uint32_t lane,
      uint32_t speed,
      std::optional<bool> adminStateOpt = true) {
    std::vector<uint32_t> lanes;
    lanes.push_back(lane);
    return SaiPortTraits::CreateAttributes{lanes,
                                           speed,
                                           adminStateOpt,
                                           std::nullopt,
                                           std::nullopt,
                                           std::nullopt,
                                           std::nullopt,
                                           std::nullopt,
                                           std::nullopt,
                                           std::nullopt};
  }

  PortSaiId createPort(uint32_t lane) {
    SaiPortTraits::CreateAttributes c = makeAttrs(lane, 100000);
    return saiApiTable->portApi().create<SaiPortTraits>(c, 0);
  }
};

TEST_F(PortStoreTest, loadPort) {
  // create a port
  auto id = createPort(0);

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiPortTraits>();

  std::vector<uint32_t> lanes{0};
  SaiPortTraits::AdapterHostKey k{lanes};
  auto got = store.get(k);
  EXPECT_EQ(got->adapterKey(), id);
}

TEST_F(PortStoreTest, portLoadCtor) {
  auto portId = createPort(0);
  SaiObject<SaiPortTraits> portObj(portId);
  EXPECT_EQ(portObj.adapterKey(), portId);
  EXPECT_EQ(GET_ATTR(Port, Speed, portObj.attributes()), 100000);
  EXPECT_EQ(GET_OPT_ATTR(Port, AdminState, portObj.attributes()), true);
}

TEST_F(PortStoreTest, portCreateCtor) {
  SaiPortTraits::CreateAttributes attrs = makeAttrs(0, 100000);
  SaiPortTraits::AdapterHostKey adapterHostKey =
      std::get<SaiPortTraits::Attributes::HwLaneList>(attrs);
  SaiObject<SaiPortTraits> obj(adapterHostKey, attrs, 0);
  auto apiSpeed = saiApiTable->portApi().getAttribute(
      obj.adapterKey(), SaiPortTraits::Attributes::Speed{});
  EXPECT_EQ(apiSpeed, 100000);
}

TEST_F(PortStoreTest, portSetSpeed) {
  auto portId = createPort(0);
  SaiObject<SaiPortTraits> portObj(portId);
  EXPECT_EQ(GET_ATTR(Port, Speed, portObj.attributes()), 100000);
  auto newAttrs = makeAttrs(0, 25000);
  portObj.setAttributes(newAttrs);
  EXPECT_EQ(GET_ATTR(Port, Speed, portObj.attributes()), 25000);
  auto apiSpeed = saiApiTable->portApi().getAttribute(
      portId, SaiPortTraits::Attributes::Speed{});
  EXPECT_EQ(apiSpeed, 25000);
}

TEST_F(PortStoreTest, portSetAdminState) {
  auto portId = createPort(0);
  SaiObject<SaiPortTraits> portObj(portId);
  EXPECT_EQ(GET_OPT_ATTR(Port, AdminState, portObj.attributes()), true);
  auto newAttrs = makeAttrs(0, 25000, false);
  portObj.setAttributes(newAttrs);
  EXPECT_EQ(GET_OPT_ATTR(Port, AdminState, portObj.attributes()), false);
  auto apiAdminState = saiApiTable->portApi().getAttribute(
      portId, SaiPortTraits::Attributes::AdminState{});
  EXPECT_EQ(apiAdminState, false);
}

TEST_F(PortStoreTest, portUnsetAdminState) {
  auto portId = createPort(0);
  SaiObject<SaiPortTraits> portObj(portId);
  EXPECT_EQ(GET_OPT_ATTR(Port, AdminState, portObj.attributes()), true);
  auto newAttrs = makeAttrs(0, 25000, std::nullopt);
  portObj.setAttributes(newAttrs);
  /*
  auto apiAdminState = saiApiTable->portApi().getAttribute(
      portId, SaiPortTraits::Attributes::AdminState{});
   * Once we properly handle un-setting and defaults, un-setting admin state
   * should make it false.
  EXPECT_EQ(apiAdminState, false);
  */
}

TEST_F(PortStoreTest, portSetPreempasis) {
  auto portId = createPort(0);
  SaiObject<SaiPortTraits> portObj(portId);
  EXPECT_EQ(
      GET_OPT_ATTR(Port, Preemphasis, portObj.attributes()),
      std::vector<uint32_t>{});
  auto newAttrs = makeAttrs(0, 25000);
  const std::vector<uint32_t> kPreemphasis{42, 43};
  std::get<std::optional<SaiPortTraits::Attributes::Preemphasis>>(newAttrs) =
      kPreemphasis;
  portObj.setAttributes(newAttrs);
  EXPECT_EQ(
      GET_OPT_ATTR(Port, Preemphasis, portObj.attributes()), kPreemphasis);
  auto apiPreemphasis = saiApiTable->portApi().getAttribute(
      portId, SaiPortTraits::Attributes::Preemphasis{});
  EXPECT_EQ(apiPreemphasis, kPreemphasis);
}

TEST_F(PortStoreTest, portSetMtu) {
  auto portId = createPort(0);
  SaiObject<SaiPortTraits> portObj(portId);
  EXPECT_EQ(GET_OPT_ATTR(Port, Mtu, portObj.attributes()), 1514);
  auto newAttrs = makeAttrs(0, 25000);
  constexpr sai_uint32_t kMtu{9000};
  std::get<std::optional<SaiPortTraits::Attributes::Mtu>>(newAttrs) = kMtu;
  portObj.setAttributes(newAttrs);
  EXPECT_EQ(GET_OPT_ATTR(Port, Mtu, portObj.attributes()), kMtu);
  auto apiMtu = saiApiTable->portApi().getAttribute(
      portId, SaiPortTraits::Attributes::Mtu{});
  EXPECT_EQ(apiMtu, kMtu);
}

/*
 * Confirm that moving out of a SaiObject<SaiPortTraits> works as expected
 */
TEST_F(PortStoreTest, testMove) {
  auto portId = createPort(0);
  SaiObject<SaiPortTraits> portObj(portId);
  {
    SaiObject<SaiPortTraits> portObj2(std::move(portObj));
    auto apiSpeed = saiApiTable->portApi().getAttribute(
        portId, SaiPortTraits::Attributes::Speed{});
    EXPECT_EQ(apiSpeed, 100000);
  }
  // port should be deleted
  // TODO(borisb): fix FakeSai to return the correct status code when the
  // object is missing instead of throwing a map::at exception. That will
  // let us improve the expected exception to SaiApiError
  EXPECT_THROW(
      saiApiTable->portApi().getAttribute(
          portId, SaiPortTraits::Attributes::Speed{}),
      std::exception);
}

TEST_F(PortStoreTest, serDeserStore) {
  // create a port
  auto id = createPort(0);

  SaiStore s(0);
  s.reload();
  auto json = s.adapterKeysFollyDynamic();
  auto& store = s.get<SaiPortTraits>();

  auto portKeysDeser = keysForSaiObjStoreFromStoreJson<SaiPortTraits>(json);
  // find, because fake sai creates a CPU port to mimic the ASIC and
  // that shows up in port store
  EXPECT_TRUE(
      std::find(portKeysDeser.begin(), portKeysDeser.end(), id) !=
      portKeysDeser.end());
}
