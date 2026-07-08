// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include <fmt/format.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
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
        ledBlocks = {},
    const std::map<std::string, std::string>& symbolicLinks = {},
    const std::string& bspKmodsRpmName = "fboss_bsp_kmods") {
  platform::platform_manager::PciDeviceConfig pciDevice;
  pciDevice.ledCtrlBlockConfigs() = ledBlocks;

  platform::platform_manager::PmUnitConfig pmUnit;
  pmUnit.pciDeviceConfigs() = {pciDevice};

  platform::platform_manager::PlatformConfig pmConfig;
  pmConfig.platformName() = platformName;
  pmConfig.numXcvrs() = numXcvrs;
  pmConfig.symbolicLinkToDevicePath() = symbolicLinks;
  pmConfig.pmUnitConfigs() = {{"test_unit", pmUnit}};
  pmConfig.bspKmodsRpmName() = bspKmodsRpmName;
  return pmConfig;
}

// Standard 4-transceiver test config:
//   Ports 1-3: 2 LEDs, 8 lanes each
//   Port 4: 1 LED, 4 lanes
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

// Fake host-state reader so getResetHoldHi() is hermetic (no rpm/uname shell).
// Overrides only the low-level virtual reads; getInstalledBspVersion() (the
// logic under test) runs for real off these.
class FakeSystemInterface
    : public platform::platform_manager::package_manager::SystemInterface {
 public:
  std::string kernelVersion;
  std::vector<std::string> rpms;
  mutable int getInstalledRpmsCallCount{0};

  std::string getHostKernelVersion() const override {
    return kernelVersion;
  }
  std::vector<std::string> getInstalledRpms(
      const std::string& /* rpmBaseName */) const override {
    ++getInstalledRpmsCallCount;
    return rpms;
  }
};

platform::platform_manager::PlatformConfig makeAristaTestConfig() {
  return makeTestConfig(
      "TEST_PLATFORM", 1, {makeLedBlock(1, 1, 2, 8)}, {}, "arista_bsp_kmods");
}

} // namespace

// --- Smoke test: ConfigLib lookup with the stable sample config ---

TEST(XcvrLibTest, ConfigLibLookupSample) {
  XcvrLib xcvr("sample");
  EXPECT_GT(xcvr.getNumTransceivers(), 0);
  EXPECT_GT(xcvr.getNumLedsForTransceiver(1), 0);
}

TEST(XcvrLibTest, ConstructFromParsedSampleConfig) {
  std::string json = platform::ConfigLib().getPlatformManagerConfig("sample");
  platform::platform_manager::PlatformConfig pmConfig;
  apache::thrift::SimpleJSONSerializer::deserialize(json, pmConfig);
  XcvrLib xcvr(pmConfig);
  EXPECT_GT(xcvr.getNumTransceivers(), 0);
  EXPECT_GT(xcvr.getNumLedsForTransceiver(1), 0);
}

// --- Detailed tests using constructed configs ---

TEST(XcvrLibTest, NumTransceivers) {
  XcvrLib xcvr(makeStandardTestConfig());
  EXPECT_EQ(xcvr.getNumTransceivers(), 4);
}

TEST(XcvrLibTest, GetXcvrIODevicePath) {
  XcvrLib xcvr(makeStandardTestConfig());
  EXPECT_EQ(xcvr.getXcvrIODevicePath(1), "/run/devmap/xcvrs/xcvr_io_1");
  EXPECT_EQ(xcvr.getXcvrIODevicePath(3), "/run/devmap/xcvrs/xcvr_io_3");
}

TEST(XcvrLibTest, GetXcvrResetSysfsPath) {
  XcvrLib xcvr(makeStandardTestConfig());
  EXPECT_EQ(
      xcvr.getXcvrResetSysfsPath(1),
      "/run/devmap/xcvrs/xcvr_ctrl_1/xcvr_reset_1");
  EXPECT_EQ(
      xcvr.getXcvrResetSysfsPath(3),
      "/run/devmap/xcvrs/xcvr_ctrl_3/xcvr_reset_3");
}

TEST(XcvrLibTest, GetXcvrPresenceSysfsPath) {
  XcvrLib xcvr(makeStandardTestConfig());
  EXPECT_EQ(
      xcvr.getXcvrPresenceSysfsPath(1),
      "/run/devmap/xcvrs/xcvr_ctrl_1/xcvr_present_1");
}

TEST(XcvrLibTest, GetPrimaryLedColorDefault) {
  XcvrLib xcvr(makeTestConfig("TEST_PLATFORM", 1, {makeLedBlock(1, 1, 2, 8)}));
  EXPECT_EQ(xcvr.getPrimaryLedColor(), XcvrLib::LedColor::BLUE);
}

TEST(XcvrLibTest, GetPrimaryLedColorDarwin) {
  XcvrLib xcvr(makeTestConfig("DARWIN", 1, {makeLedBlock(1, 1, 2, 8)}));
  EXPECT_EQ(xcvr.getPrimaryLedColor(), XcvrLib::LedColor::GREEN);
}

TEST(XcvrLibTest, GetPrimaryLedColorDarwin48v) {
  XcvrLib xcvr(makeTestConfig("DARWIN48V", 1, {makeLedBlock(1, 1, 2, 8)}));
  EXPECT_EQ(xcvr.getPrimaryLedColor(), XcvrLib::LedColor::GREEN);
}

TEST(XcvrLibTest, GetLedSysfsPath) {
  XcvrLib xcvr(makeStandardTestConfig());
  EXPECT_EQ(
      xcvr.getLedSysfsPath(1, 1, XcvrLib::LedColor::BLUE),
      "/sys/class/leds/port1_led1:blue:status");
  EXPECT_EQ(
      xcvr.getLedSysfsPath(3, 2, XcvrLib::LedColor::BLUE),
      "/sys/class/leds/port3_led2:blue:status");
  EXPECT_EQ(
      xcvr.getLedSysfsPath(1, 1, XcvrLib::LedColor::AMBER),
      "/sys/class/leds/port1_led1:amber:status");
  EXPECT_EQ(
      xcvr.getLedSysfsPath(1, 1, XcvrLib::LedColor::GREEN),
      "/sys/class/leds/port1_led1:green:status");
}

TEST(XcvrLibTest, GetNumLedsForTransceiver) {
  XcvrLib xcvr(makeStandardTestConfig());
  // Ports 1-3 have 2 LEDs
  EXPECT_EQ(xcvr.getNumLedsForTransceiver(1), 2);
  EXPECT_EQ(xcvr.getNumLedsForTransceiver(3), 2);
  // Port 4 has 1 LED
  EXPECT_EQ(xcvr.getNumLedsForTransceiver(4), 1);
}

TEST(XcvrLibTest, ConstructorThrowsForUncoveredXcvrs) {
  // LED blocks exist but don't cover all ports — constructor throws
  auto config = makeTestConfig("TEST_PLATFORM", 3, {makeLedBlock(1, 1, 1, 4)});
  EXPECT_THROW(XcvrLib{config}, std::runtime_error);
}

TEST(XcvrLibTest, ConstructorThrowsForEmptyLedBlocks) {
  // No LED blocks at all with xcvrs present — constructor throws
  auto config = makeTestConfig("TEST_PLATFORM", 3, {});
  EXPECT_THROW(XcvrLib{config}, std::runtime_error);
}

TEST(XcvrLibTest, GetNumLanesForTransceiver) {
  XcvrLib xcvr(makeStandardTestConfig());
  // Ports 1-3 have 8 lanes
  EXPECT_EQ(xcvr.getNumLanesForTransceiver(1), 8);
  EXPECT_EQ(xcvr.getNumLanesForTransceiver(3), 8);
  // Port 4 has 4 lanes
  EXPECT_EQ(xcvr.getNumLanesForTransceiver(4), 4);
}

TEST(XcvrLibTest, GetResetMask) {
  XcvrLib xcvr(makeStandardTestConfig());
  EXPECT_EQ(xcvr.getResetMask(), 1);
}

TEST(XcvrLibTest, GetPresenceMask) {
  XcvrLib xcvr(makeStandardTestConfig());
  EXPECT_EQ(xcvr.getPresenceMask(), 1);
}

TEST(XcvrLibTest, GetResetHoldHiFbossBsp) {
  XcvrLib xcvr(makeTestConfig(
      "TEST_PLATFORM", 1, {makeLedBlock(1, 1, 2, 8)}, {}, "fboss_bsp_kmods"));
  EXPECT_EQ(xcvr.getResetHoldHi(), 1);
}

// The version strings below are mock fixtures, not real BSP releases; they only
// need to sort below/above the source threshold to exercise both polarity
// paths.
constexpr auto kMockKernel = "mock_kernel_running";
constexpr auto kMockOtherKernel = "mock_kernel_other";
constexpr auto kMockVersionBelowThreshold = "0.0.1";
constexpr auto kMockVersionAboveThreshold = "9.9.9";

std::string makeMockBspRpm(
    const std::string& kernel,
    const std::string& bspVer) {
  return fmt::format("arista_bsp_kmods-{}-{}-1.x86_64", kernel, bspVer);
}

TEST(XcvrLibTest, GetResetHoldHiAristaBelowThreshold) {
  // Installed BSP sorts below the threshold -> active-low.
  auto fake = std::make_shared<FakeSystemInterface>();
  fake->kernelVersion = kMockKernel;
  fake->rpms = {makeMockBspRpm(kMockKernel, kMockVersionBelowThreshold)};
  XcvrLib xcvr(makeAristaTestConfig(), fake);
  EXPECT_EQ(xcvr.getResetHoldHi(), 0);
}

TEST(XcvrLibTest, GetResetHoldHiAristaAtOrAboveThreshold) {
  // Installed BSP sorts at/above the threshold -> active-high.
  auto fake = std::make_shared<FakeSystemInterface>();
  fake->kernelVersion = kMockKernel;
  fake->rpms = {makeMockBspRpm(kMockKernel, kMockVersionAboveThreshold)};
  XcvrLib xcvr(makeAristaTestConfig(), fake);
  EXPECT_EQ(xcvr.getResetHoldHi(), 1);
}

TEST(XcvrLibTest, GetResetHoldHiAristaPicksRunningKernel) {
  // Two BSP RPMs installed (two kernel variants); polarity is derived from the
  // running kernel's RPM (above threshold), ignoring the other kernel's RPM.
  auto fake = std::make_shared<FakeSystemInterface>();
  fake->kernelVersion = kMockKernel;
  fake->rpms = {
      makeMockBspRpm(kMockOtherKernel, kMockVersionBelowThreshold),
      makeMockBspRpm(kMockKernel, kMockVersionAboveThreshold)};
  XcvrLib xcvr(makeAristaTestConfig(), fake);
  EXPECT_EQ(xcvr.getResetHoldHi(), 1);
}

TEST(XcvrLibTest, GetResetHoldHiAristaNoMatchFailsSafe) {
  // No installed RPM matches the running kernel -> fail safe to active-low.
  auto fake = std::make_shared<FakeSystemInterface>();
  fake->kernelVersion = kMockKernel;
  fake->rpms = {makeMockBspRpm(kMockOtherKernel, kMockVersionAboveThreshold)};
  XcvrLib xcvr(makeAristaTestConfig(), fake);
  EXPECT_EQ(xcvr.getResetHoldHi(), 0);
}

TEST(XcvrLibTest, GetResetHoldHiCachesAcrossCalls) {
  // The mapping build calls getResetHoldHi() once per transceiver against one
  // XcvrLib instance; the host-state read must happen only once, not per call.
  auto fake = std::make_shared<FakeSystemInterface>();
  fake->kernelVersion = kMockKernel;
  fake->rpms = {makeMockBspRpm(kMockKernel, kMockVersionAboveThreshold)};
  XcvrLib xcvr(makeAristaTestConfig(), fake);
  xcvr.getResetHoldHi();
  xcvr.getResetHoldHi();
  xcvr.getResetHoldHi();
  EXPECT_EQ(fake->getInstalledRpmsCallCount, 1);
}

TEST(XcvrLibTest, GetResetHoldHiCiscoBsp) {
  XcvrLib xcvr(makeTestConfig(
      "TEST_PLATFORM", 1, {makeLedBlock(1, 1, 2, 8)}, {}, "cisco_bsp_kmods"));
  EXPECT_EQ(xcvr.getResetHoldHi(), 0);
}

// --- Tests for std::nullopt return paths (invalid xcvrId) ---

TEST(XcvrLibTest, GetNumLedsForTransceiverInvalidId) {
  XcvrLib xcvr(makeStandardTestConfig());
  EXPECT_EQ(xcvr.getNumLedsForTransceiver(0), std::nullopt);
  EXPECT_EQ(xcvr.getNumLedsForTransceiver(-1), std::nullopt);
  EXPECT_EQ(xcvr.getNumLedsForTransceiver(5), std::nullopt);
}

TEST(XcvrLibTest, GetNumLanesForTransceiverInvalidId) {
  XcvrLib xcvr(makeStandardTestConfig());
  EXPECT_EQ(xcvr.getNumLanesForTransceiver(0), std::nullopt);
  EXPECT_EQ(xcvr.getNumLanesForTransceiver(-1), std::nullopt);
  EXPECT_EQ(xcvr.getNumLanesForTransceiver(5), std::nullopt);
}

TEST(XcvrLibTest, GetXcvrIODevicePathInvalidId) {
  XcvrLib xcvr(makeStandardTestConfig());
  EXPECT_EQ(xcvr.getXcvrIODevicePath(0), std::nullopt);
  EXPECT_EQ(xcvr.getXcvrIODevicePath(5), std::nullopt);
}

TEST(XcvrLibTest, GetXcvrResetSysfsPathInvalidId) {
  XcvrLib xcvr(makeStandardTestConfig());
  EXPECT_EQ(xcvr.getXcvrResetSysfsPath(0), std::nullopt);
  EXPECT_EQ(xcvr.getXcvrResetSysfsPath(5), std::nullopt);
}

TEST(XcvrLibTest, GetXcvrPresenceSysfsPathInvalidId) {
  XcvrLib xcvr(makeStandardTestConfig());
  EXPECT_EQ(xcvr.getXcvrPresenceSysfsPath(0), std::nullopt);
  EXPECT_EQ(xcvr.getXcvrPresenceSysfsPath(5), std::nullopt);
}

TEST(XcvrLibTest, GetLedSysfsPathInvalidXcvrId) {
  XcvrLib xcvr(makeStandardTestConfig());
  EXPECT_EQ(xcvr.getLedSysfsPath(0, 1, XcvrLib::LedColor::BLUE), std::nullopt);
  EXPECT_EQ(xcvr.getLedSysfsPath(5, 1, XcvrLib::LedColor::BLUE), std::nullopt);
}

TEST(XcvrLibTest, GetLedSysfsPathInvalidLedNum) {
  XcvrLib xcvr(makeStandardTestConfig());
  // Port 1 has 2 LEDs, so ledNum 0 and 3 are invalid
  EXPECT_EQ(xcvr.getLedSysfsPath(1, 0, XcvrLib::LedColor::BLUE), std::nullopt);
  EXPECT_EQ(xcvr.getLedSysfsPath(1, 3, XcvrLib::LedColor::BLUE), std::nullopt);
}
