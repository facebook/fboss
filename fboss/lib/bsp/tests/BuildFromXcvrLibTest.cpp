// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include <folly/FileUtil.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include "fboss/lib/bsp/BuildFromXcvrLib.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"
#include "fboss/lib/led/gen-cpp2/led_mapping_types.h"
#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/xcvr_lib/XcvrLib.h"

using namespace facebook::fboss;

namespace {

platform::platform_manager::LedCtrlBlockConfig
makeLedBlock(int startPort, int numPorts, int ledPerPort, int lanesPerPort) {
  platform::platform_manager::LedCtrlBlockConfig block;
  block.startPort() = startPort;
  block.numPorts() = numPorts;
  block.ledPerPort() = ledPerPort;
  block.lanesPerPort() = lanesPerPort;
  return block;
}

platform::platform_manager::PlatformConfig makeTestConfig(
    const std::string& platformName,
    int numXcvrs,
    const std::vector<platform::platform_manager::LedCtrlBlockConfig>&
        ledBlocks,
    const std::map<std::string, std::string>& symbolicLinks) {
  platform::platform_manager::PciDeviceConfig pciDevice;
  pciDevice.ledCtrlBlockConfigs() = ledBlocks;

  platform::platform_manager::PmUnitConfig pmUnit;
  pmUnit.pciDeviceConfigs() = {pciDevice};

  platform::platform_manager::PlatformConfig pmConfig;
  pmConfig.platformName() = platformName;
  pmConfig.numXcvrs() = numXcvrs;
  pmConfig.symbolicLinkToDevicePath() = symbolicLinks;
  pmConfig.pmUnitConfigs() = {{"test_unit", pmUnit}};
  return pmConfig;
}

// 4 transceivers:
//   Ports 1-3: 2 LEDs, 8 lanes each
//   Port 4:    1 LED,  4 lanes
platform::platform_manager::PlatformConfig makeStandardTestConfig() {
  return makeTestConfig(
      "TEST_PLATFORM",
      4,
      {makeLedBlock(1, 3, 2, 8), makeLedBlock(4, 1, 1, 4)},
      {{"/run/devmap/xcvrs/xcvr_io_1", "/dev/1"},
       {"/run/devmap/xcvrs/xcvr_ctrl_1", "/dev/ctrl1"},
       {"/run/devmap/xcvrs/xcvr_io_2", "/dev/2"},
       {"/run/devmap/xcvrs/xcvr_ctrl_2", "/dev/ctrl2"},
       {"/run/devmap/xcvrs/xcvr_io_3", "/dev/3"},
       {"/run/devmap/xcvrs/xcvr_ctrl_3", "/dev/ctrl3"},
       {"/run/devmap/xcvrs/xcvr_io_4", "/dev/4"},
       {"/run/devmap/xcvrs/xcvr_ctrl_4", "/dev/ctrl4"}});
}

} // namespace

// --- Basic structure tests ---

TEST(BuildFromXcvrLibTest, SinglePimWithCorrectId) {
  XcvrLib xcvr(makeStandardTestConfig());
  auto result = buildFromXcvrLib(xcvr);
  ASSERT_EQ(result.pimMapping()->size(), 1);
  ASSERT_TRUE(result.pimMapping()->count(1));
  EXPECT_EQ(*result.pimMapping()->at(1).pimID(), 1);
}

TEST(BuildFromXcvrLibTest, EmptyPhyMappings) {
  XcvrLib xcvr(makeStandardTestConfig());
  auto result = buildFromXcvrLib(xcvr);
  const auto& pim = result.pimMapping()->at(1);
  EXPECT_TRUE(pim.phyMapping()->empty());
  EXPECT_TRUE(pim.phyIOControllers()->empty());
}

TEST(BuildFromXcvrLibTest, TransceiverCount) {
  XcvrLib xcvr(makeStandardTestConfig());
  auto result = buildFromXcvrLib(xcvr);
  const auto& pim = result.pimMapping()->at(1);
  EXPECT_EQ(pim.tcvrMapping()->size(), 4);
}

// --- Transceiver access control tests ---

TEST(BuildFromXcvrLibTest, TransceiverAccessControlFields) {
  XcvrLib xcvr(makeStandardTestConfig());
  auto result = buildFromXcvrLib(xcvr);
  const auto& tcvr1 = result.pimMapping()->at(1).tcvrMapping()->at(1);

  const auto& ac = *tcvr1.accessControl();
  EXPECT_EQ(*ac.controllerId(), "1");
  EXPECT_EQ(*ac.type(), ResetAndPresenceAccessType::CPLD);
  EXPECT_EQ(*ac.gpioChip(), "");

  // Reset pin
  const auto& reset = *ac.reset();
  EXPECT_EQ(*reset.sysfsPath(), "/run/devmap/xcvrs/xcvr_ctrl_1/xcvr_reset_1");
  EXPECT_EQ(*reset.mask(), 1);
  EXPECT_EQ(*reset.gpioOffset(), 0);
  EXPECT_EQ(*reset.resetHoldHi(), 1); // default resetHoldHi

  // Presence pin
  const auto& presence = *ac.presence();
  EXPECT_EQ(
      *presence.sysfsPath(), "/run/devmap/xcvrs/xcvr_ctrl_1/xcvr_present_1");
  EXPECT_EQ(*presence.mask(), 1);
  EXPECT_EQ(*presence.gpioOffset(), 0);
  EXPECT_EQ(*presence.presentHoldHi(), 0);
}

// --- Transceiver IO control tests ---

TEST(BuildFromXcvrLibTest, TransceiverIOControlFields) {
  XcvrLib xcvr(makeStandardTestConfig());
  auto result = buildFromXcvrLib(xcvr);
  const auto& tcvr1 = result.pimMapping()->at(1).tcvrMapping()->at(1);

  const auto& io = *tcvr1.io();
  EXPECT_EQ(*io.controllerId(), "1");
  EXPECT_EQ(*io.type(), TransceiverIOType::I2C);
  EXPECT_EQ(*io.devicePath(), "/run/devmap/xcvrs/xcvr_io_1");
}

// --- Lane-to-LED mapping tests ---

TEST(BuildFromXcvrLibTest, LaneToLedMapping8Lanes2Leds) {
  XcvrLib xcvr(makeStandardTestConfig());
  auto result = buildFromXcvrLib(xcvr);
  const auto& tcvr1 = result.pimMapping()->at(1).tcvrMapping()->at(1);
  const auto& laneMap = *tcvr1.tcvrLaneToLedId();

  // Port 1: 8 lanes, 2 LEDs -> 4 lanes per LED
  // LED IDs for port 1 are 1 and 2
  ASSERT_EQ(laneMap.size(), 8);
  // Lanes 1-4 -> LED 1
  EXPECT_EQ(laneMap.at(1), 1);
  EXPECT_EQ(laneMap.at(4), 1);
  // Lanes 5-8 -> LED 2
  EXPECT_EQ(laneMap.at(5), 2);
  EXPECT_EQ(laneMap.at(8), 2);
}

TEST(BuildFromXcvrLibTest, LaneToLedMapping4Lanes1Led) {
  XcvrLib xcvr(makeStandardTestConfig());
  auto result = buildFromXcvrLib(xcvr);
  // Port 4: 4 lanes, 1 LED -> all lanes map to LED 7
  // (ports 1-3 have 2 LEDs each = 6 LEDs, so port 4 starts at LED 7)
  const auto& tcvr4 = result.pimMapping()->at(1).tcvrMapping()->at(4);
  const auto& laneMap = *tcvr4.tcvrLaneToLedId();

  ASSERT_EQ(laneMap.size(), 4);
  for (int lane = 1; lane <= 4; ++lane) {
    EXPECT_EQ(laneMap.at(lane), 7);
  }
}

TEST(BuildFromXcvrLibTest, LaneToLedMappingConsecutivePorts) {
  XcvrLib xcvr(makeStandardTestConfig());
  auto result = buildFromXcvrLib(xcvr);
  const auto& pim = result.pimMapping()->at(1);

  // Port 2 LEDs should start at 3 (after port 1's 2 LEDs)
  const auto& tcvr2 = pim.tcvrMapping()->at(2);
  EXPECT_EQ(tcvr2.tcvrLaneToLedId()->at(1), 3);
  EXPECT_EQ(tcvr2.tcvrLaneToLedId()->at(5), 4);

  // Port 3 LEDs should start at 5 (after port 1-2's 4 LEDs)
  const auto& tcvr3 = pim.tcvrMapping()->at(3);
  EXPECT_EQ(tcvr3.tcvrLaneToLedId()->at(1), 5);
  EXPECT_EQ(tcvr3.tcvrLaneToLedId()->at(5), 6);
}

// --- LED mapping tests ---

TEST(BuildFromXcvrLibTest, TotalLedCount) {
  XcvrLib xcvr(makeStandardTestConfig());
  auto result = buildFromXcvrLib(xcvr);
  const auto& pim = result.pimMapping()->at(1);

  // Ports 1-3: 2 LEDs each = 6, Port 4: 1 LED = 7 total
  EXPECT_EQ(pim.ledMapping()->size(), 7);
}

TEST(BuildFromXcvrLibTest, LedMappingFields) {
  XcvrLib xcvr(makeStandardTestConfig());
  auto result = buildFromXcvrLib(xcvr);
  const auto& leds = *result.pimMapping()->at(1).ledMapping();

  // LED 1: port 1, led 1
  const auto& led1 = leds.at(1);
  EXPECT_EQ(*led1.id(), 1);
  EXPECT_EQ(*led1.transceiverId(), 1);
  EXPECT_EQ(*led1.bluePath(), "/sys/class/leds/port1_led1:blue:status");
  EXPECT_EQ(*led1.yellowPath(), "/sys/class/leds/port1_led1:amber:status");

  // LED 2: port 1, led 2
  const auto& led2 = leds.at(2);
  EXPECT_EQ(*led2.id(), 2);
  EXPECT_EQ(*led2.transceiverId(), 1);
  EXPECT_EQ(*led2.bluePath(), "/sys/class/leds/port1_led2:blue:status");
  EXPECT_EQ(*led2.yellowPath(), "/sys/class/leds/port1_led2:amber:status");
}

TEST(BuildFromXcvrLibTest, LedMappingConsecutiveIds) {
  XcvrLib xcvr(makeStandardTestConfig());
  auto result = buildFromXcvrLib(xcvr);
  const auto& leds = *result.pimMapping()->at(1).ledMapping();

  // LED 3: port 2, led 1
  EXPECT_EQ(*leds.at(3).transceiverId(), 2);
  EXPECT_EQ(*leds.at(3).bluePath(), "/sys/class/leds/port2_led1:blue:status");

  // LED 7: port 4, led 1 (single LED port)
  EXPECT_EQ(*leds.at(7).transceiverId(), 4);
  EXPECT_EQ(*leds.at(7).bluePath(), "/sys/class/leds/port4_led1:blue:status");
}

// --- LED-to-transceiver consistency test ---

TEST(BuildFromXcvrLibTest, LedIdsConsistentBetweenTcvrAndLedMappings) {
  XcvrLib xcvr(makeStandardTestConfig());
  auto result = buildFromXcvrLib(xcvr);
  const auto& pim = result.pimMapping()->at(1);
  const auto& leds = *pim.ledMapping();

  // Every LED ID referenced in tcvrLaneToLedId must exist in ledMapping
  for (const auto& [tcvrId, tcvr] : *pim.tcvrMapping()) {
    for (const auto& [lane, ledId] : *tcvr.tcvrLaneToLedId()) {
      EXPECT_TRUE(leds.count(ledId))
          << "Transceiver " << tcvrId << " lane " << lane << " references LED "
          << ledId << " which doesn't exist";
      // The LED should reference this transceiver
      EXPECT_EQ(*leds.at(ledId).transceiverId(), tcvrId)
          << "LED " << ledId << " references transceiver "
          << *leds.at(ledId).transceiverId() << " but expected " << tcvrId;
    }
  }
}

// --- All transceivers created when config is valid ---

TEST(BuildFromXcvrLibTest, AllTransceiversCreated) {
  XcvrLib xcvr(makeStandardTestConfig());
  auto result = buildFromXcvrLib(xcvr);
  const auto& pim = result.pimMapping()->at(1);

  for (int tcvrId = 1; tcvrId <= 4; ++tcvrId) {
    EXPECT_TRUE(pim.tcvrMapping()->count(tcvrId))
        << "Transceiver " << tcvrId << " missing";
  }
}

// --- Sample platform fixture comparison test ---

TEST(BuildFromXcvrLibTest, SamplePlatformMatchesFixture) {
  auto constructed = buildFromXcvrLib(XcvrLib("sample"));

  std::string json;
  std::string path =
      "fboss/lib/bsp/tests/bsp_platform_mapping_json/sample.json";
  ASSERT_TRUE(folly::readFile(path.c_str(), json))
      << "Failed to read JSON fixture: " << path;

  BspPlatformMappingThrift expected;
  apache::thrift::SimpleJSONSerializer::deserialize<BspPlatformMappingThrift>(
      json, expected);

  EXPECT_EQ(constructed, expected)
      << "Constructed BSP mapping for sample platform does not match fixture";
}
