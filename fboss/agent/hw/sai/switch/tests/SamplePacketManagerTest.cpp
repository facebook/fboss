/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSamplePacketManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/types.h"

using namespace facebook::fboss;
class SamplePacketManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::BLANK;
    ManagerTestBase::SetUp();
    p0 = testInterfaces[0].remoteHosts[0].port;
    p1 = testInterfaces[1].remoteHosts[0].port;
  }
  TestInterface intf0;
  TestRemoteHost h0;
  TestPort p0;
  TestPort p1;

  std::shared_ptr<SaiSamplePacket> createSamplePacket(
      uint16_t sampleRate = 100,
      cfg::SampleDestination sampleDestination = cfg::SampleDestination::CPU) {
    return saiManagerTable->samplePacketManager().getOrCreateSamplePacket(
        sampleRate, sampleDestination);
  }

  void checkSamplePacket(
      const std::shared_ptr<SaiSamplePacket> samplePacket,
      uint16_t sampleRate = 100,
      cfg::SampleDestination sampleDestination = cfg::SampleDestination::CPU) {
    auto& samplePacketApi = saiApiTable->samplePacketApi();
    auto gotSampleRate = samplePacketApi.getAttribute(
        samplePacket->adapterKey(),
        SaiSamplePacketTraits::Attributes::SampleRate{});
    EXPECT_EQ(gotSampleRate, sampleRate);
    auto gotType = samplePacketApi.getAttribute(
        samplePacket->adapterKey(), SaiSamplePacketTraits::Attributes::Type{});
    sai_samplepacket_type_t expectedSampleType =
        SAI_SAMPLEPACKET_TYPE_SLOW_PATH;
    if (sampleDestination == cfg::SampleDestination::MIRROR) {
      expectedSampleType = SAI_SAMPLEPACKET_TYPE_MIRROR_SESSION;
    }
    EXPECT_EQ(gotType, expectedSampleType);
  }
};

TEST_F(SamplePacketManagerTest, createSamplePacket) {
  auto samplePacket = createSamplePacket();
  checkSamplePacket(samplePacket);
}

TEST_F(SamplePacketManagerTest, createDuplicateSamplePacket) {
  auto samplePacket1 = createSamplePacket();
  auto samplePacket2 = createSamplePacket();
  EXPECT_EQ(samplePacket1->adapterKey(), samplePacket2->adapterKey());
}

TEST_F(SamplePacketManagerTest, portSamplingToCpu) {
  std::shared_ptr<Port> swPort1 = makePort(p0);
  swPort1->setSflowIngressRate(100);
  swPort1->setSampleDestination(cfg::SampleDestination::CPU);
  saiManagerTable->portManager().addPort(swPort1);
  SaiPortHandle* portHandle =
      saiManagerTable->portManager().getPortHandle(swPort1->getID());
  auto& portApi = saiApiTable->portApi();
  auto gotMirrorSaiIdList = portApi.getAttribute(
      portHandle->port->adapterKey(),
      SaiPortTraits::Attributes::IngressMirrorSession{});
  EXPECT_EQ(gotMirrorSaiIdList.size(), 0);
  auto gotSamplePacketSaiId = portApi.getAttribute(
      portHandle->port->adapterKey(),
      SaiPortTraits::Attributes::IngressSamplePacketEnable{});
  EXPECT_EQ(
      gotSamplePacketSaiId, portHandle->ingressSamplePacket->adapterKey());
}
