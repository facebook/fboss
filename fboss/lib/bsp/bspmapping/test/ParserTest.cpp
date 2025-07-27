// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/bspmapping/Parser.h"
#include <folly/Range.h>
#include <gtest/gtest.h>
#include "fboss/lib/if/gen-cpp2/fboss_common_types.h"

using namespace ::testing;

TEST(ParserTest, GetNameForTests) {
  EXPECT_EQ(
      facebook::fboss::Parser::getNameFor(
          facebook::fboss::PlatformType::PLATFORM_MONTBLANC),
      "montblanc");
  EXPECT_EQ(
      facebook::fboss::Parser::getNameFor(
          facebook::fboss::PlatformType::PLATFORM_MINIPACK3N),
      "minipack3n");
  EXPECT_EQ(
      facebook::fboss::Parser::getNameFor(
          facebook::fboss::PlatformType::PLATFORM_MERU400BFU),
      "meru400bfu");
  EXPECT_EQ(
      facebook::fboss::Parser::getNameFor(
          facebook::fboss::PlatformType::PLATFORM_MERU400BIU),
      "meru400biu");
  EXPECT_EQ(
      facebook::fboss::Parser::getNameFor(
          facebook::fboss::PlatformType::PLATFORM_MERU800BIA),
      "meru800bia");
  EXPECT_EQ(
      facebook::fboss::Parser::getNameFor(
          facebook::fboss::PlatformType::PLATFORM_MERU800BIAB),
      "meru800biab");
  EXPECT_EQ(
      facebook::fboss::Parser::getNameFor(
          facebook::fboss::PlatformType::PLATFORM_MERU800BIAC),
      "meru800biac");
  EXPECT_EQ(
      facebook::fboss::Parser::getNameFor(
          facebook::fboss::PlatformType::PLATFORM_MERU800BFA),
      "meru800bfa");
  EXPECT_EQ(
      facebook::fboss::Parser::getNameFor(
          facebook::fboss::PlatformType::PLATFORM_JANGA800BIC),
      "janga800bic");
  EXPECT_EQ(
      facebook::fboss::Parser::getNameFor(
          facebook::fboss::PlatformType::PLATFORM_TAHAN800BC),
      "tahan800bc");
  EXPECT_EQ(
      facebook::fboss::Parser::getNameFor(
          facebook::fboss::PlatformType::PLATFORM_MORGAN800CC),
      "morgan800cc");
  EXPECT_EQ(
      facebook::fboss::Parser::getNameFor(
          facebook::fboss::PlatformType::PLATFORM_ICECUBE800BC),
      "icecube");
}

TEST(ParserTest, GetTransceiverConfigRowFromCsvLine) {
  // First line taken from Montblanc_BspMapping.csv
  std::string line =
      "1,1 2 3 4,1,1,CPLD,/run/devmap/xcvrs/xcvr_1/xcvr_reset_1,1,1,/run/devmap/xcvrs/xcvr_1/xcvr_present_1,1,0,1,I2C,/run/devmap/i2c-busses/XCVR_1,1,/sys/class/leds/port1_led1:blue:status,/sys/class/leds/port1_led1:yellow:status";
  auto transceiverConfigRow =
      facebook::fboss::Parser::getTransceiverConfigRowFromCsvLine(line);
  EXPECT_EQ(transceiverConfigRow.get_tcvrId(), 1);
  EXPECT_TRUE(transceiverConfigRow.get_tcvrLaneIdList() != nullptr);
  EXPECT_EQ(
      *transceiverConfigRow.get_tcvrLaneIdList(),
      std::vector<int>({1, 2, 3, 4}));
  EXPECT_EQ(transceiverConfigRow.get_pimId(), 1);
  EXPECT_EQ(transceiverConfigRow.get_accessCtrlId(), "1");
  EXPECT_EQ(
      transceiverConfigRow.get_accessCtrlType(),
      facebook::fboss::ResetAndPresenceAccessType::CPLD);
  EXPECT_EQ(
      transceiverConfigRow.get_resetPath(),
      "/run/devmap/xcvrs/xcvr_1/xcvr_reset_1");
  EXPECT_EQ(transceiverConfigRow.get_resetMask(), 1);
  EXPECT_EQ(transceiverConfigRow.get_resetHoldHi(), 1);
  EXPECT_EQ(
      transceiverConfigRow.get_presentPath(),
      "/run/devmap/xcvrs/xcvr_1/xcvr_present_1");
  EXPECT_EQ(transceiverConfigRow.get_presentMask(), 1);
  EXPECT_EQ(transceiverConfigRow.get_presentHoldHi(), 0);
  EXPECT_EQ(transceiverConfigRow.get_ioCtrlId(), "1");
  EXPECT_EQ(
      transceiverConfigRow.get_ioCtrlType(),
      facebook::fboss::TransceiverIOType::I2C);
  EXPECT_EQ(transceiverConfigRow.get_ioPath(), "/run/devmap/i2c-busses/XCVR_1");
  EXPECT_TRUE(transceiverConfigRow.get_ledId() != nullptr);
  EXPECT_EQ(*transceiverConfigRow.get_ledId(), 1);
  EXPECT_TRUE(transceiverConfigRow.get_ledBluePath() != nullptr);
  EXPECT_EQ(
      *transceiverConfigRow.get_ledBluePath(),
      "/sys/class/leds/port1_led1:blue:status");
  EXPECT_TRUE(transceiverConfigRow.get_ledYellowPath() != nullptr);
  EXPECT_EQ(
      *transceiverConfigRow.get_ledYellowPath(),
      "/sys/class/leds/port1_led1:yellow:status");
}

TEST(ParserTest, GetTransceiverConfigRowFromCsvLineNullCorrect) {
  // First line taken from Meru400bfu_BspMapping.csv
  std::string line =
      "1,,1,accessController-1,CPLD,/sys/bus/i2c/devices/2-0032/cpld_qsfpdd_port_config_0,1,0,/sys/bus/i2c/devices/2-0032/cpld_qsfpdd_port_status_0,2,0,ioController-1,I2C,/dev/i2c-21,,,,";
  auto transceiverConfigRow =
      facebook::fboss::Parser::getTransceiverConfigRowFromCsvLine(line);
  EXPECT_EQ(transceiverConfigRow.get_tcvrId(), 1);
  EXPECT_TRUE(transceiverConfigRow.get_tcvrLaneIdList() == nullptr);
  EXPECT_EQ(transceiverConfigRow.get_pimId(), 1);
  EXPECT_EQ(transceiverConfigRow.get_accessCtrlId(), "accessController-1");
  EXPECT_EQ(
      transceiverConfigRow.get_accessCtrlType(),
      facebook::fboss::ResetAndPresenceAccessType::CPLD);
  EXPECT_EQ(
      transceiverConfigRow.get_resetPath(),
      "/sys/bus/i2c/devices/2-0032/cpld_qsfpdd_port_config_0");
  EXPECT_EQ(transceiverConfigRow.get_resetMask(), 1);
  EXPECT_EQ(transceiverConfigRow.get_resetHoldHi(), 0);
  EXPECT_EQ(
      transceiverConfigRow.get_presentPath(),
      "/sys/bus/i2c/devices/2-0032/cpld_qsfpdd_port_status_0");
  EXPECT_EQ(transceiverConfigRow.get_presentMask(), 2);
  EXPECT_EQ(transceiverConfigRow.get_presentHoldHi(), 0);
  EXPECT_EQ(transceiverConfigRow.get_ioCtrlId(), "ioController-1");
  EXPECT_EQ(
      transceiverConfigRow.get_ioCtrlType(),
      facebook::fboss::TransceiverIOType::I2C);
  EXPECT_EQ(transceiverConfigRow.get_ioPath(), "/dev/i2c-21");
  EXPECT_TRUE(transceiverConfigRow.get_ledId() == nullptr);
  EXPECT_TRUE(transceiverConfigRow.get_ledBluePath() == nullptr);
  EXPECT_TRUE(transceiverConfigRow.get_ledYellowPath() == nullptr);
}

TEST(ParserTest, GetTransceiverConfigRowFromCsvLineThrowsOnMalformedLine) {
  // First line taken from Meru400bfu_BspMapping.csv, but it's missing 3 fields
  // for LED attributes.
  std::string line =
      "1,,1,accessController-1,CPLD,/sys/bus/i2c/devices/2-0032/cpld_qsfpdd_port_config_0,1,0,/sys/bus/i2c/devices/2-0032/cpld_qsfpdd_port_status_0,2,0,ioController-1,I2C,/dev/i2c-21";
  EXPECT_THROW(
      facebook::fboss::Parser::getTransceiverConfigRowFromCsvLine(line),
      std::runtime_error);
}

TEST(ParserTest, GetTransceiverConfigRowsFromCsvTest) {
  auto transceivers = facebook::fboss::Parser::getTransceiverConfigRowsFromCsv(
      folly::StringPiece(
          "fboss/lib/bsp/bspmapping/test/test_data/test_example.csv"));
  EXPECT_EQ(transceivers.size(), 2);
}

TEST(ParserTest, GetBspPlatformMappingFromCsvTest) {
  auto bspPlatformMapping =
      facebook::fboss::Parser::getBspPlatformMappingFromCsv(folly::StringPiece(
          "fboss/lib/bsp/bspmapping/test/test_data/test_example.csv"));
  EXPECT_EQ(bspPlatformMapping.get_pimMapping().size(), 1);
  EXPECT_TRUE(
      bspPlatformMapping.get_pimMapping().find(1) !=
      bspPlatformMapping.get_pimMapping().end());
  EXPECT_EQ(bspPlatformMapping.get_pimMapping().at(1).get_pimID(), 1);
  auto tcvrMapping =
      bspPlatformMapping.pimMapping().value().at(1).tcvrMapping().value();
  EXPECT_EQ(tcvrMapping.size(), 1);
  EXPECT_TRUE(tcvrMapping.find(1) != tcvrMapping.end());
  EXPECT_EQ(tcvrMapping.at(1).get_tcvrId(), 1);
  EXPECT_EQ(tcvrMapping.at(1).get_accessControl().get_controllerId(), "1");
  EXPECT_EQ(
      tcvrMapping.at(1).get_accessControl().get_type(),
      facebook::fboss::ResetAndPresenceAccessType::CPLD);

  EXPECT_TRUE(
      tcvrMapping.at(1).get_accessControl().get_reset().get_sysfsPath() !=
      nullptr);
  EXPECT_EQ(
      *tcvrMapping.at(1).get_accessControl().get_reset().get_sysfsPath(),
      "/run/devmap/xcvrs/xcvr_1/xcvr_reset_1");
  EXPECT_TRUE(
      tcvrMapping.at(1).get_accessControl().get_reset().get_mask() != nullptr);
  EXPECT_EQ(*tcvrMapping.at(1).get_accessControl().get_reset().get_mask(), 1);
  EXPECT_TRUE(
      tcvrMapping.at(1).get_accessControl().get_reset().get_gpioOffset() !=
      nullptr);
  EXPECT_EQ(
      *tcvrMapping.at(1).get_accessControl().get_reset().get_gpioOffset(), 0);
  EXPECT_TRUE(
      tcvrMapping.at(1).get_accessControl().get_reset().get_resetHoldHi() !=
      nullptr);
  EXPECT_EQ(
      *tcvrMapping.at(1).get_accessControl().get_reset().get_resetHoldHi(), 1);

  EXPECT_TRUE(
      tcvrMapping.at(1).get_accessControl().get_presence().get_sysfsPath() !=
      nullptr);
  EXPECT_EQ(
      *tcvrMapping.at(1).get_accessControl().get_presence().get_sysfsPath(),
      "/run/devmap/cplds/JANGA_SMB_CPLD/xcvr_present_1");
  EXPECT_TRUE(
      tcvrMapping.at(1).get_accessControl().get_presence().get_mask() !=
      nullptr);
  EXPECT_EQ(
      *tcvrMapping.at(1).get_accessControl().get_presence().get_mask(), 1);
  EXPECT_TRUE(
      tcvrMapping.at(1).get_accessControl().get_presence().get_gpioOffset() !=
      nullptr);
  EXPECT_EQ(
      *tcvrMapping.at(1).get_accessControl().get_presence().get_gpioOffset(),
      0);
  EXPECT_TRUE(
      tcvrMapping.at(1)
          .get_accessControl()
          .get_presence()
          .get_presentHoldHi() != nullptr);
  EXPECT_EQ(
      *tcvrMapping.at(1).get_accessControl().get_presence().get_presentHoldHi(),
      1);

  EXPECT_TRUE(tcvrMapping.at(1).get_accessControl().get_gpioChip() != nullptr);
  EXPECT_EQ(*tcvrMapping.at(1).get_accessControl().get_gpioChip(), "");

  EXPECT_EQ(tcvrMapping.at(1).get_io().get_controllerId(), "1");
  EXPECT_EQ(
      tcvrMapping.at(1).get_io().get_type(),
      facebook::fboss::TransceiverIOType::I2C);
  EXPECT_EQ(
      tcvrMapping.at(1).get_io().get_devicePath(),
      "/run/devmap/i2c-busses/XCVR_1");

  EXPECT_EQ(tcvrMapping.at(1).get_tcvrLaneToLedId().size(), 4);
  std::map<int, int> expectedLaneToLedId = {{1, 1}, {2, 1}, {3, 1}, {4, 1}};
  EXPECT_EQ(tcvrMapping.at(1).get_tcvrLaneToLedId(), expectedLaneToLedId);

  EXPECT_TRUE(
      bspPlatformMapping.get_pimMapping().at(1).get_phyMapping().empty());
  EXPECT_TRUE(
      bspPlatformMapping.get_pimMapping().at(1).get_phyIOControllers().empty());

  EXPECT_EQ(
      bspPlatformMapping.get_pimMapping().at(1).get_ledMapping().size(), 1);
  auto ledMapping =
      bspPlatformMapping.pimMapping().value().at(1).ledMapping().value().at(1);

  EXPECT_EQ(ledMapping.get_id(), 1);
  EXPECT_TRUE(ledMapping.get_bluePath() != nullptr);
  EXPECT_EQ(
      *ledMapping.get_bluePath(), "/sys/class/leds/port1_led1:blue:status");
  EXPECT_TRUE(ledMapping.get_yellowPath() != nullptr);
  EXPECT_EQ(
      *ledMapping.get_yellowPath(), "/sys/class/leds/port1_led1:yellow:status");
  EXPECT_EQ(ledMapping.get_transceiverId(), 1);
}
