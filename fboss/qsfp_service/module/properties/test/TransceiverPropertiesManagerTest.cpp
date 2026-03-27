// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/module/properties/TransceiverPropertiesManager.h"

#include <folly/FileUtil.h>
#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>
#include "fboss/agent/FbossError.h"

using namespace facebook::fboss;

class TransceiverPropertiesManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    TransceiverPropertiesManager::reset();
  }

  void TearDown() override {
    TransceiverPropertiesManager::reset();
  }

  void writeConfigAndInit(const std::string& json) {
    tmpFile_ = std::make_unique<folly::test::TemporaryFile>();
    folly::writeFile(json, tmpFile_->path().c_str());
    TransceiverPropertiesManager::init(tmpFile_->path().string());
  }

  void initDefault() {
    TransceiverPropertiesManager::initDefault();
  }

 private:
  std::unique_ptr<folly::test::TemporaryFile> tmpFile_;
};

TEST_F(TransceiverPropertiesManagerTest, NotInitialized) {
  EXPECT_FALSE(TransceiverPropertiesManager::isInitialized());
  EXPECT_FALSE(
      TransceiverPropertiesManager::isKnown(MediaInterfaceCode::DR4_2x800G));
  EXPECT_THROW(
      TransceiverPropertiesManager::getNumHostLanes(
          MediaInterfaceCode::DR4_2x800G),
      FbossError);
  EXPECT_THROW(
      TransceiverPropertiesManager::deriveSmfCode(0x77, {0, 4}, 0x25, 500),
      FbossError);
}

TEST_F(TransceiverPropertiesManagerTest, LoadDefaultConfig) {
  initDefault();
  EXPECT_TRUE(TransceiverPropertiesManager::isInitialized());
}

TEST_F(TransceiverPropertiesManagerTest, DefaultConfigAllCodesKnown) {
  initDefault();

  EXPECT_TRUE(
      TransceiverPropertiesManager::isKnown(MediaInterfaceCode::FR4_2x400G));
  EXPECT_TRUE(
      TransceiverPropertiesManager::isKnown(MediaInterfaceCode::DR4_2x400G));
  EXPECT_TRUE(
      TransceiverPropertiesManager::isKnown(
          MediaInterfaceCode::FR4_LITE_2x400G));
  EXPECT_TRUE(
      TransceiverPropertiesManager::isKnown(
          MediaInterfaceCode::LR4_2x400G_10KM));
  EXPECT_TRUE(
      TransceiverPropertiesManager::isKnown(MediaInterfaceCode::DR4_2x800G));
  EXPECT_TRUE(
      TransceiverPropertiesManager::isKnown(
          MediaInterfaceCode::FR4_LPO_2x400G));
}

TEST_F(TransceiverPropertiesManagerTest, DR4_2x800G_Properties) {
  initDefault();

  auto code = MediaInterfaceCode::DR4_2x800G;
  EXPECT_EQ(TransceiverPropertiesManager::getNumHostLanes(code), 8);
  EXPECT_EQ(TransceiverPropertiesManager::getNumMediaLanes(code), 8);
}

TEST_F(TransceiverPropertiesManagerTest, TwoPort400G_Properties) {
  initDefault();

  for (auto code :
       {MediaInterfaceCode::FR4_2x400G,
        MediaInterfaceCode::DR4_2x400G,
        MediaInterfaceCode::FR4_LITE_2x400G,
        MediaInterfaceCode::LR4_2x400G_10KM,
        MediaInterfaceCode::FR4_LPO_2x400G}) {
    EXPECT_EQ(TransceiverPropertiesManager::getNumHostLanes(code), 8)
        << "Failed for code " << static_cast<int>(code);
    EXPECT_EQ(TransceiverPropertiesManager::getNumMediaLanes(code), 8)
        << "Failed for code " << static_cast<int>(code);
  }
}

TEST_F(TransceiverPropertiesManagerTest, DeriveSmfCodeDR4_2x800G) {
  initDefault();

  EXPECT_EQ(
      TransceiverPropertiesManager::deriveSmfCode(0x77, {0, 4}, 0x82, 500),
      MediaInterfaceCode::DR4_2x800G);
}

TEST_F(TransceiverPropertiesManagerTest, DeriveSmfCodeFR4_2x400G) {
  initDefault();

  EXPECT_EQ(
      TransceiverPropertiesManager::deriveSmfCode(0x1D, {0, 4}, 0x50, 3000),
      MediaInterfaceCode::FR4_2x400G);
}

TEST_F(TransceiverPropertiesManagerTest, DeriveSmfCodeFR4_LITE_2x400G) {
  initDefault();

  EXPECT_EQ(
      TransceiverPropertiesManager::deriveSmfCode(0x1D, {0, 4}, 0x50, 500),
      MediaInterfaceCode::FR4_LITE_2x400G);
}

TEST_F(TransceiverPropertiesManagerTest, DeriveSmfCodeDR4_2x400G) {
  initDefault();

  EXPECT_EQ(
      TransceiverPropertiesManager::deriveSmfCode(0x1C, {0, 4}, 0x50, 500),
      MediaInterfaceCode::DR4_2x400G);
}

TEST_F(TransceiverPropertiesManagerTest, DeriveSmfCodeLR4_2x400G_10KM) {
  initDefault();

  EXPECT_EQ(
      TransceiverPropertiesManager::deriveSmfCode(0x1E, {0, 4}, 0x50, 10000),
      MediaInterfaceCode::LR4_2x400G_10KM);
}

TEST_F(TransceiverPropertiesManagerTest, DeriveSmfCodeUnknown) {
  initDefault();

  // Unknown SMF byte
  EXPECT_EQ(
      TransceiverPropertiesManager::deriveSmfCode(0xFF, {0}, 0x25, 500),
      MediaInterfaceCode::UNKNOWN);

  // Known SMF byte but wrong number of hostStartLanes
  EXPECT_EQ(
      TransceiverPropertiesManager::deriveSmfCode(0x77, {0, 1, 2}, 0x25, 500),
      MediaInterfaceCode::UNKNOWN);

  // Known SMF byte + hostStartLanes but wrong hostInterfaceCode
  EXPECT_EQ(
      TransceiverPropertiesManager::deriveSmfCode(0x77, {0, 4}, 0xFF, 500),
      MediaInterfaceCode::UNKNOWN);

  // Known SMF byte + hostStartLanes + hostInterfaceCode but wrong smfLength
  EXPECT_EQ(
      TransceiverPropertiesManager::deriveSmfCode(0x77, {0, 4}, 0x82, 9999),
      MediaInterfaceCode::UNKNOWN);
}

TEST_F(TransceiverPropertiesManagerTest, DeriveSmfCodeLpoNotInDerivation) {
  initDefault();

  // FR4_LPO_2x400G has empty hostStartLanes in config, so it should NOT
  // be matched by derivation.
  auto result =
      TransceiverPropertiesManager::deriveSmfCode(0x1D, {0, 4}, 0x50, 3000);
  EXPECT_NE(result, MediaInterfaceCode::FR4_LPO_2x400G);
  EXPECT_EQ(result, MediaInterfaceCode::FR4_2x400G);
}

TEST_F(TransceiverPropertiesManagerTest, UnknownCodeThrows) {
  initDefault();

  auto code = MediaInterfaceCode::FR4_400G;
  EXPECT_FALSE(TransceiverPropertiesManager::isKnown(code));
  EXPECT_THROW(TransceiverPropertiesManager::getNumHostLanes(code), FbossError);
  EXPECT_THROW(
      TransceiverPropertiesManager::getNumMediaLanes(code), FbossError);
}

TEST_F(TransceiverPropertiesManagerTest, GetProperties) {
  initDefault();

  const auto& props = TransceiverPropertiesManager::getProperties(
      MediaInterfaceCode::DR4_2x800G);
  EXPECT_EQ(*props.displayName(), "DR4_2x800G");

  EXPECT_THROW(
      TransceiverPropertiesManager::getProperties(MediaInterfaceCode::FR4_400G),
      FbossError);
}

TEST_F(TransceiverPropertiesManagerTest, GetPropertiesByIntValue) {
  initDefault();

  EXPECT_TRUE(TransceiverPropertiesManager::isKnown(23));
  const auto& props = TransceiverPropertiesManager::getProperties(23);
  EXPECT_EQ(*props.displayName(), "DR4_2x800G");

  EXPECT_FALSE(TransceiverPropertiesManager::isKnown(99));
  EXPECT_THROW(TransceiverPropertiesManager::getProperties(99), FbossError);
}

TEST_F(TransceiverPropertiesManagerTest, SpeedCombinations) {
  initDefault();

  auto combos =
      TransceiverPropertiesManager::getSpeedCombinations<SMFMediaInterfaceCode>(
          MediaInterfaceCode::DR4_2x800G);

  // First combo (map-sorted): 2x800G-DR4 — all lanes DR4_800G (0x77)
  for (int i = 0; i < 8; i++) {
    EXPECT_EQ(combos[0][i], SMFMediaInterfaceCode::DR4_800G);
  }

  EXPECT_THROW(
      TransceiverPropertiesManager::getSpeedCombinations<SMFMediaInterfaceCode>(
          MediaInterfaceCode::FR4_400G),
      FbossError);
}

TEST_F(TransceiverPropertiesManagerTest, DiagsCapsDefaultSupported) {
  initDefault();

  auto code = MediaInterfaceCode::DR4_2x800G;
  EXPECT_FALSE(TransceiverPropertiesManager::getDoesNotSupportVdm(code));
  EXPECT_FALSE(
      TransceiverPropertiesManager::getDoesNotSupportRxOutputControl(code));
}

TEST_F(TransceiverPropertiesManagerTest, DiagsCapsExplicitlyUnsupported) {
  std::string config = R"({
    "smfTransceivers": {
      "99": {
        "firstApplicationAdvertisement": {
          "mediaInterfaceCode": {"smfCode": 0x50},
          "hostStartLanes": [0],
          "hostInterfaceCode": 0x20
        },
        "smfLength": 500,
        "numHostLanes": 4,
        "numMediaLanes": 4,
        "displayName": "NO_VDM_OPTIC",
        "diagnosticCapabilitiesExceptions": {
          "doesNotSupportVdm": true,
          "doesNotSupportRxOutputControl": true
        },
        "supportedSpeedCombinations": []
      }
    }
  })";
  writeConfigAndInit(config);

  EXPECT_TRUE(
      TransceiverPropertiesManager::getDoesNotSupportVdm(
          static_cast<MediaInterfaceCode>(99)));
  EXPECT_TRUE(
      TransceiverPropertiesManager::getDoesNotSupportRxOutputControl(
          static_cast<MediaInterfaceCode>(99)));
}

TEST_F(TransceiverPropertiesManagerTest, FindSpeedCombination) {
  initDefault();

  auto code = MediaInterfaceCode::DR4_2x800G;

  const auto& combo =
      TransceiverPropertiesManager::findSpeedCombination(code, "2x800G-DR4");
  EXPECT_EQ(*combo.combinationName(), "2x800G-DR4");

  EXPECT_THROW(
      TransceiverPropertiesManager::findSpeedCombination(code, "nonexistent"),
      FbossError);

  EXPECT_THROW(
      TransceiverPropertiesManager::findSpeedCombination(
          MediaInterfaceCode::FR4_400G, "2x800G-DR4"),
      FbossError);
}

TEST_F(TransceiverPropertiesManagerTest, SpeedChangeTransitionValidation) {
  std::string badConfig = R"({
    "smfTransceivers": {
      "23": {
        "firstApplicationAdvertisement": {
          "mediaInterfaceCode": {"smfCode": 0x77},
          "hostStartLanes": [0, 4],
          "hostInterfaceCode": 0x25
        },
        "smfLength": 500,
        "numHostLanes": 8,
        "numMediaLanes": 8,
        "displayName": "DR4_2x800G",
        "supportedSpeedCombinations": [
          {
            "combinationName": "2x800G-DR4",
            "ports": [
              {"speed": 800000, "hostLanes": {"start": 0, "count": 4}, "mediaLanes": {"start": 0, "count": 4}, "mediaLaneCode": {"smfCode": 0x77}},
              {"speed": 800000, "hostLanes": {"start": 4, "count": 4}, "mediaLanes": {"start": 4, "count": 4}, "mediaLaneCode": {"smfCode": 0x77}}
            ]
          }
        ],
        "speedChangeTransitions": [["2x800G-DR4", "NONEXISTENT"]]
      }
    }
  })";
  EXPECT_THROW(writeConfigAndInit(badConfig), FbossError);
  EXPECT_FALSE(TransceiverPropertiesManager::isInitialized());
}

TEST_F(TransceiverPropertiesManagerTest, ValidSpeedChangeTransitions) {
  initDefault();

  const auto& props = TransceiverPropertiesManager::getProperties(
      MediaInterfaceCode::DR4_2x800G);
  EXPECT_TRUE(props.speedChangeTransitions()->empty());
}

TEST_F(TransceiverPropertiesManagerTest, MalformedConfigWrongFieldName) {
  // Using "smfCode" instead of "mediaInterfaceCode" — the field is silently
  // ignored by LENIENT Thrift parsing, leaving mediaInterfaceCode as an empty
  // union. Validation should catch this and throw.
  std::string badConfig = R"({
    "smfTransceivers": {
      "23": {
        "firstApplicationAdvertisement": {
          "smfCode": 0x77,
          "hostStartLanes": [0, 4],
          "hostInterfaceCode": 0x82
        },
        "smfLength": 500,
        "numHostLanes": 8,
        "numMediaLanes": 8,
        "displayName": "DR4_2x800G",
        "supportedSpeedCombinations": []
      }
    }
  })";
  EXPECT_THROW(writeConfigAndInit(badConfig), FbossError);
  EXPECT_FALSE(TransceiverPropertiesManager::isInitialized());
}

TEST_F(TransceiverPropertiesManagerTest, InvalidConfigPath) {
  EXPECT_THROW(
      TransceiverPropertiesManager::init("/nonexistent/path.json"), FbossError);
  EXPECT_FALSE(TransceiverPropertiesManager::isInitialized());
}

TEST_F(TransceiverPropertiesManagerTest, MediaLaneCodeToMediaInterfaceCode) {
  initDefault();

  // Known mappings from the default config
  EXPECT_EQ(
      TransceiverPropertiesManager::mediaLaneCodeToMediaInterfaceCode(0x1D),
      MediaInterfaceCode::FR4_400G);
  EXPECT_EQ(
      TransceiverPropertiesManager::mediaLaneCodeToMediaInterfaceCode(0x18),
      MediaInterfaceCode::FR4_200G);
  EXPECT_EQ(
      TransceiverPropertiesManager::mediaLaneCodeToMediaInterfaceCode(0x15),
      MediaInterfaceCode::FR1_100G);
  EXPECT_EQ(
      TransceiverPropertiesManager::mediaLaneCodeToMediaInterfaceCode(0x10),
      MediaInterfaceCode::CWDM4_100G);
  EXPECT_EQ(
      TransceiverPropertiesManager::mediaLaneCodeToMediaInterfaceCode(0xC1),
      MediaInterfaceCode::FR8_800G);
  EXPECT_EQ(
      TransceiverPropertiesManager::mediaLaneCodeToMediaInterfaceCode(0x1C),
      MediaInterfaceCode::DR4_400G);
  EXPECT_EQ(
      TransceiverPropertiesManager::mediaLaneCodeToMediaInterfaceCode(0x14),
      MediaInterfaceCode::DR1_100G);
  EXPECT_EQ(
      TransceiverPropertiesManager::mediaLaneCodeToMediaInterfaceCode(0x1E),
      MediaInterfaceCode::LR4_400G_10KM);
  EXPECT_EQ(
      TransceiverPropertiesManager::mediaLaneCodeToMediaInterfaceCode(0x19),
      MediaInterfaceCode::LR4_200G);
  EXPECT_EQ(
      TransceiverPropertiesManager::mediaLaneCodeToMediaInterfaceCode(0x77),
      MediaInterfaceCode::DR4_800G);
  EXPECT_EQ(
      TransceiverPropertiesManager::mediaLaneCodeToMediaInterfaceCode(0x73),
      MediaInterfaceCode::DR1_200G);
  EXPECT_EQ(
      TransceiverPropertiesManager::mediaLaneCodeToMediaInterfaceCode(0x75),
      MediaInterfaceCode::DR2_400G);
  EXPECT_EQ(
      TransceiverPropertiesManager::mediaLaneCodeToMediaInterfaceCode(0x68),
      MediaInterfaceCode::ZR_800G);
  EXPECT_EQ(
      TransceiverPropertiesManager::mediaLaneCodeToMediaInterfaceCode(0x6C),
      MediaInterfaceCode::ZR_800G);
  EXPECT_EQ(
      TransceiverPropertiesManager::mediaLaneCodeToMediaInterfaceCode(0xF7),
      MediaInterfaceCode::ZR_800G);

  // Unknown code throws
  EXPECT_THROW(
      TransceiverPropertiesManager::mediaLaneCodeToMediaInterfaceCode(0xFF),
      FbossError);
}

TEST_F(TransceiverPropertiesManagerTest, MediaLaneCodeNotInitialized) {
  EXPECT_THROW(
      TransceiverPropertiesManager::mediaLaneCodeToMediaInterfaceCode(0x1D),
      FbossError);
}

TEST_F(TransceiverPropertiesManagerTest, Reset) {
  initDefault();
  EXPECT_TRUE(TransceiverPropertiesManager::isInitialized());
  EXPECT_TRUE(
      TransceiverPropertiesManager::isKnown(MediaInterfaceCode::DR4_2x800G));

  TransceiverPropertiesManager::reset();
  EXPECT_FALSE(TransceiverPropertiesManager::isInitialized());
  EXPECT_FALSE(
      TransceiverPropertiesManager::isKnown(MediaInterfaceCode::DR4_2x800G));
}
