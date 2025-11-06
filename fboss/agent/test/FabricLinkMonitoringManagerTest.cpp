/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/FabricLinkMonitoringManager.h"

#include <fb303/ServiceData.h>
#include <folly/Memory.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/mock/MockHwSwitch.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"
#include "gmock/gmock.h"

using ::testing::_;
using ::testing::AtLeast;

using namespace facebook::fboss;
using folly::MacAddress;
using folly::io::Cursor;
using folly::io::RWPrivateCursor;
using std::make_shared;
using std::shared_ptr;
using std::unique_ptr;

namespace {

constexpr size_t kFabricLinkMonitoringPacketSize = 480;

unique_ptr<HwTestHandle> setupTestHandle() {
  auto state = testStateAWithPortsUp();
  addSwitchInfo(
      state,
      cfg::SwitchType::FABRIC,
      0,
      cfg::AsicType::ASIC_TYPE_MOCK,
      cfg::switch_config_constants::DEFAULT_PORT_ID_RANGE_MIN(),
      cfg::switch_config_constants::DEFAULT_PORT_ID_RANGE_MAX(),
      0,
      std::nullopt,
      std::nullopt,
      MockPlatform::getMockLocalMac().toString());

  for (auto& portMap : std::as_const(*state->getPorts())) {
    for (auto& port : std::as_const(*portMap.second)) {
      auto newPort = port.second->modify(&state);
      newPort->setPortType(cfg::PortType::FABRIC_PORT);
    }
  }

  return createTestHandle(state);
}

TxMatchFn checkFabricMonitoringPacket() {
  return [=](const TxPacket* pkt) {
    const auto* buf = pkt->buf();
    const auto chainlen = buf->computeChainDataLength();

    EXPECT_EQ(chainlen, kFabricLinkMonitoringPacketSize);

    Cursor c(buf);

    auto sequenceNumber = c.readBE<uint64_t>();
    EXPECT_GE(sequenceNumber, 0);

    auto portId = c.readBE<uint32_t>();
    EXPECT_GE(portId, 0);

    auto payloadPattern =
        FabricLinkMonitoringManager::getPayloadPattern(sequenceNumber);
    size_t payloadSize =
        kFabricLinkMonitoringPacketSize - sizeof(uint64_t) - sizeof(uint32_t);
    for (size_t i = 0; i < payloadSize; i += 4) {
      auto value = c.readBE<uint32_t>();
      EXPECT_EQ(value, payloadPattern);
    }
  };
}

std::unique_ptr<RxPacket> createFabricMonitoringRxPacket(
    SwSwitch* /*sw*/,
    const PortID& portId,
    uint64_t sequenceNumber) {
  std::vector<uint8_t> data(kFabricLinkMonitoringPacketSize);

  // Serialize sequenceNumber (uint64_t) in big-endian order
  uint64_t seq_be = htobe64(sequenceNumber);
  std::memcpy(data.data(), &seq_be, sizeof(seq_be));

  // Serialize portId (uint32_t) in big-endian order
  uint32_t port_be = htobe32(static_cast<uint32_t>(portId));
  std::memcpy(data.data() + 8, &port_be, sizeof(port_be));

  // Fill the rest of the buffer with pattern
  size_t fillStart = sizeof(uint64_t) + sizeof(uint32_t);
  ;
  auto fillPattern =
      FabricLinkMonitoringManager::getPayloadPattern(sequenceNumber);
  for (size_t i = fillStart; i + 4 <= kFabricLinkMonitoringPacketSize; i += 4) {
    std::memcpy(data.data() + i, &fillPattern, sizeof(fillPattern));
  }

  auto buf = folly::IOBuf::copyBuffer(data.data(), data.size());
  auto rxPkt = std::make_unique<MockRxPacket>(std::move(buf));
  rxPkt->setSrcPort(portId);
  return rxPkt;
}

} // namespace

TEST(FabricLinkMonitoringManagerTest, CreateManager) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();

  auto manager = std::make_unique<FabricLinkMonitoringManager>(sw);
  EXPECT_NE(manager, nullptr);
}

TEST(FabricLinkMonitoringManagerTest, SendPacketsOnFabricPorts) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();

  EXPECT_HW_CALL(
      sw,
      sendPacketOutOfPortSyncForPktType_(
          TxPacketMatcher::createMatcher(
              "Fabric Monitoring Packet", checkFabricMonitoringPacket()),
          _,
          _))
      .Times(AtLeast(1));

  auto manager = std::make_unique<FabricLinkMonitoringManager>(sw);
  manager->start();

  waitForStateUpdates(sw);
  waitForBackgroundThread(sw);

  manager->stop();
}
