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
 protected:
  SaiPortTraits::CreateAttributes makeAttrs(
      uint32_t lane,
      uint32_t speed,
      std::optional<bool> adminStateOpt = true) {
    std::vector<uint32_t> lanes;
    lanes.push_back(lane);
    return SaiPortTraits::CreateAttributes{
        lanes,        speed,        adminStateOpt, std::nullopt,
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
        std::nullopt, std::nullopt,
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 11, 0)
        std::nullopt, // Port Fabric Isolate
#endif
        std::nullopt, std::nullopt, std::nullopt,  std::nullopt, std::nullopt,
        std::nullopt, std::nullopt, std::nullopt,  std::nullopt, std::nullopt,
        std::nullopt, // Ingress Mirror Session
        std::nullopt, // Egress Mirror Session
        std::nullopt, // Ingress Sample Packet
        std::nullopt, // Egress Sample Packet
        std::nullopt, // Ingress mirror sample session
        std::nullopt, // Egress mirror sample session
        std::nullopt, // PRBS Polynomial
        std::nullopt, // PRBS Config
        std::nullopt, // Ingress macsec acl
        std::nullopt, // Egress macsec acl
        std::nullopt, // System Port Id
        std::nullopt, // PTP Mode
        std::nullopt, // PFC Mode
        std::nullopt, // PFC Priorities
#if !defined(TAJO_SDK)
        std::nullopt, // PFC Rx Priorities
        std::nullopt, // PFC Tx Priorities
#endif
        std::nullopt, // TC to Priority Group map
        std::nullopt, // PFC Priority to Queue map
#if SAI_API_VERSION >= SAI_VERSION(1, 9, 0)
        std::nullopt, // Inter Frame Gap
#endif
        std::nullopt, // Link Training Enable
        std::nullopt, // FDR Enable
        std::nullopt, // Rx Lane Squelch Enable
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
        std::nullopt, // PFC Deadlock Detection Interval
        std::nullopt, // PFC Deadlock Recovery Interval
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
        std::nullopt, // ARS enable
        std::nullopt, // ARS scaling factor
        std::nullopt, // ARS port load past weight
        std::nullopt, // ARS port load future weight
#endif
        std::nullopt, // Reachability Group
    };
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

TEST_F(PortStoreTest, loadPortFromJson) {
  // create a port
  auto id = createPort(0);

  SaiStore s(0);
  s.reload();
  auto json = s.adapterKeysFollyDynamic();
  SaiStore s2(0);
  s2.reload(&json);
  auto& store = s2.get<SaiPortTraits>();

  std::vector<uint32_t> lanes{0};
  SaiPortTraits::AdapterHostKey k{lanes};
  auto got = store.get(k);
  EXPECT_EQ(got->adapterKey(), id);
}

TEST_F(PortStoreTest, portLoadCtor) {
  auto portId = createPort(0);
  auto portObj = createObj<SaiPortTraits>(portId);
  EXPECT_EQ(portObj.adapterKey(), portId);
  EXPECT_EQ(GET_ATTR(Port, Speed, portObj.attributes()), 100000);
  EXPECT_EQ(GET_OPT_ATTR(Port, AdminState, portObj.attributes()), true);
}

TEST_F(PortStoreTest, portCreateCtor) {
  SaiPortTraits::CreateAttributes attrs = makeAttrs(0, 100000);
  SaiPortTraits::AdapterHostKey adapterHostKey =
      std::get<SaiPortTraits::Attributes::HwLaneList>(attrs);
  auto obj = createObj<SaiPortTraits>(adapterHostKey, attrs, 0);
  auto apiSpeed = saiApiTable->portApi().getAttribute(
      obj.adapterKey(), SaiPortTraits::Attributes::Speed{});
  EXPECT_EQ(apiSpeed, 100000);
}

TEST_F(PortStoreTest, portSetSpeed) {
  auto portId = createPort(0);
  SaiObject<SaiPortTraits> portObj = createObj<SaiPortTraits>(portId);
  EXPECT_EQ(GET_ATTR(Port, Speed, portObj.attributes()), 100000);
  auto newAttrs = makeAttrs(0, 25000);
  portObj.setAttributes(newAttrs);
  EXPECT_EQ(GET_ATTR(Port, Speed, portObj.attributes()), 25000);
  auto apiSpeed = saiApiTable->portApi().getAttribute(
      portId, SaiPortTraits::Attributes::Speed{});
  EXPECT_EQ(apiSpeed, 25000);
}

TEST_F(PortStoreTest, portSetOnlySpeed) {
  auto portId = createPort(0);
  SaiObject<SaiPortTraits> portObj = createObj<SaiPortTraits>(portId);
  EXPECT_EQ(GET_ATTR(Port, Speed, portObj.attributes()), 100000);
  auto apiSpeed = saiApiTable->portApi().getAttribute(
      portId, SaiPortTraits::Attributes::Speed{});
  EXPECT_EQ(apiSpeed, 100000);
  portObj.setAttribute(SaiPortTraits::Attributes::Speed{25000});
  EXPECT_EQ(GET_ATTR(Port, Speed, portObj.attributes()), 25000);
  apiSpeed = saiApiTable->portApi().getAttribute(
      portId, SaiPortTraits::Attributes::Speed{});
  EXPECT_EQ(apiSpeed, 25000);
}

TEST_F(PortStoreTest, portSetAdminState) {
  auto portId = createPort(0);
  SaiObject<SaiPortTraits> portObj = createObj<SaiPortTraits>(portId);
  EXPECT_EQ(GET_OPT_ATTR(Port, AdminState, portObj.attributes()), true);
  auto newAttrs = makeAttrs(0, 25000, false);
  portObj.setAttributes(newAttrs);
  EXPECT_EQ(GET_OPT_ATTR(Port, AdminState, portObj.attributes()), false);
  auto apiAdminState = saiApiTable->portApi().getAttribute(
      portId, SaiPortTraits::Attributes::AdminState{});
  EXPECT_EQ(apiAdminState, false);
}

TEST_F(PortStoreTest, portSetOnlyAdminState) {
  auto portId = createPort(0);
  SaiObject<SaiPortTraits> portObj = createObj<SaiPortTraits>(portId);
  auto apiAdminState = saiApiTable->portApi().getAttribute(
      portId, SaiPortTraits::Attributes::AdminState{});
  EXPECT_EQ(apiAdminState, true);
  portObj.setOptionalAttribute(SaiPortTraits::Attributes::AdminState{false});
  EXPECT_EQ(GET_OPT_ATTR(Port, AdminState, portObj.attributes()), false);
  apiAdminState = saiApiTable->portApi().getAttribute(
      portId, SaiPortTraits::Attributes::AdminState{});
  EXPECT_EQ(apiAdminState, false);
}

TEST_F(PortStoreTest, portUnsetAdminState) {
  auto portId = createPort(0);
  SaiObject<SaiPortTraits> portObj = createObj<SaiPortTraits>(portId);
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

TEST_F(PortStoreTest, portSetMtu) {
  auto portId = createPort(0);
  SaiObject<SaiPortTraits> portObj = createObj<SaiPortTraits>(portId);
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
#if SAI_API_VERSION >= SAI_VERSION(1, 11, 0)
TEST_F(PortStoreTest, portFabricIsolate) {
  auto portId = createPort(0);
  SaiObject<SaiPortTraits> portObj = createObj<SaiPortTraits>(portId);
  EXPECT_EQ(GET_OPT_ATTR(Port, FabricIsolate, portObj.attributes()), false);
  auto newAttrs = makeAttrs(0, 25000);
  constexpr bool kFabricIsolate{true};
  std::get<std::optional<SaiPortTraits::Attributes::FabricIsolate>>(newAttrs) =
      kFabricIsolate;
  portObj.setAttributes(newAttrs);
  EXPECT_EQ(
      GET_OPT_ATTR(Port, FabricIsolate, portObj.attributes()), kFabricIsolate);
  auto apiFabricIsolate = saiApiTable->portApi().getAttribute(
      portId, SaiPortTraits::Attributes::FabricIsolate{});
  EXPECT_EQ(apiFabricIsolate, kFabricIsolate);
}
#endif

TEST_F(PortStoreTest, portSetQoSMaps) {
  auto portId = createPort(0);
  SaiObject<SaiPortTraits> portObj = createObj<SaiPortTraits>(portId);
  EXPECT_EQ(GET_OPT_ATTR(Port, Mtu, portObj.attributes()), 1514);
  auto newAttrs = makeAttrs(0, 25000);
  std::get<std::optional<SaiPortTraits::Attributes::QosDscpToTcMap>>(newAttrs) =
      42;
  std::get<std::optional<SaiPortTraits::Attributes::QosTcToQueueMap>>(
      newAttrs) = 43;
  portObj.setAttributes(newAttrs);
  EXPECT_EQ(GET_OPT_ATTR(Port, QosDscpToTcMap, portObj.attributes()), 42);
  EXPECT_EQ(GET_OPT_ATTR(Port, QosTcToQueueMap, portObj.attributes()), 43);
  auto apiQosDscpToTc = saiApiTable->portApi().getAttribute(
      portId, SaiPortTraits::Attributes::QosDscpToTcMap{});
  EXPECT_EQ(apiQosDscpToTc, 42);
  auto apiQosTcToQueue = saiApiTable->portApi().getAttribute(
      portId, SaiPortTraits::Attributes::QosTcToQueueMap{});
  EXPECT_EQ(apiQosTcToQueue, 43);
}

TEST_F(PortStoreTest, portSetDisableTtl) {
  auto portId = createPort(0);
  SaiObject<SaiPortTraits> portObj = createObj<SaiPortTraits>(portId);
  EXPECT_FALSE(GET_OPT_ATTR(Port, DisableTtlDecrement, portObj.attributes()));
  auto newAttrs = makeAttrs(0, 25000);
  std::get<std::optional<SaiPortTraits::Attributes::DisableTtlDecrement>>(
      newAttrs) = true;
  portObj.setAttributes(newAttrs);
  EXPECT_TRUE(GET_OPT_ATTR(Port, DisableTtlDecrement, portObj.attributes()));
  EXPECT_TRUE(saiApiTable->portApi().getAttribute(
      portId, SaiPortTraits::Attributes::DisableTtlDecrement{}));
}
/*
 * Confirm that moving out of a SaiObject<SaiPortTraits> works as expected
 */
TEST_F(PortStoreTest, testMove) {
  auto portId = createPort(0);
  auto portObj = createObj<SaiPortTraits>(portId);
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
  // Look for this key in port adaptor JSON
  verifyAdapterKeySerDeser<SaiPortTraits>({id}, true);
}

TEST_F(PortStoreTest, toStrStore) {
  std::ignore = createPort(0);
  verifyToStr<SaiPortTraits>();
}

TEST_F(PortStoreTest, portSetPtpMode) {
  auto portId = createPort(0);
  SaiObject<SaiPortTraits> portObj = createObj<SaiPortTraits>(portId);
  EXPECT_EQ(
      GET_OPT_ATTR(Port, PtpMode, portObj.attributes()),
      SAI_PORT_PTP_MODE_NONE);
  auto newAttrs = makeAttrs(0, 25000);
  constexpr sai_uint32_t kPtpMode{SAI_PORT_PTP_MODE_SINGLE_STEP_TIMESTAMP};
  std::get<std::optional<SaiPortTraits::Attributes::PtpMode>>(newAttrs) =
      kPtpMode;
  portObj.setAttributes(newAttrs);
  EXPECT_EQ(GET_OPT_ATTR(Port, PtpMode, portObj.attributes()), kPtpMode);
  auto apiPtpMode = saiApiTable->portApi().getAttribute(
      portId, SaiPortTraits::Attributes::PtpMode{});
  EXPECT_EQ(apiPtpMode, kPtpMode);
}

TEST_F(PortStoreTest, portSetDisableLinkTraining) {
  auto portId = createPort(0);
  SaiObject<SaiPortTraits> portObj = createObj<SaiPortTraits>(portId);
  EXPECT_FALSE(GET_OPT_ATTR(Port, LinkTrainingEnable, portObj.attributes()));
  auto newAttrs = makeAttrs(0, 25000);
  std::get<std::optional<SaiPortTraits::Attributes::LinkTrainingEnable>>(
      newAttrs) = true;
  portObj.setAttributes(newAttrs);
  EXPECT_TRUE(GET_OPT_ATTR(Port, LinkTrainingEnable, portObj.attributes()));
  EXPECT_TRUE(saiApiTable->portApi().getAttribute(
      portId, SaiPortTraits::Attributes::LinkTrainingEnable{}));

  std::get<std::optional<SaiPortTraits::Attributes::LinkTrainingEnable>>(
      newAttrs) = false;
  portObj.setAttributes(newAttrs);
  EXPECT_FALSE(GET_OPT_ATTR(Port, LinkTrainingEnable, portObj.attributes()));
  EXPECT_FALSE(saiApiTable->portApi().getAttribute(
      portId, SaiPortTraits::Attributes::LinkTrainingEnable{}));
}
