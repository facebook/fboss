// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/module/tests/TransceiverTestsHelper.h"

#include <gtest/gtest.h>

namespace facebook {
namespace fboss {

void testCachedMediaSignals(QsfpModule* qsfp) {
  auto mgmtInterface =
      qsfp->getTransceiverInfo().transceiverManagementInterface_ref().value_or(
          {});
  auto writeTxFault = [&](uint8_t fault) {
    TransceiverIOParameters param;
    param.length_ref() = 1;
    if (mgmtInterface == TransceiverManagementInterface::SFF) {
      param.offset_ref() = 4;
      param.page_ref() = 0;
    } else {
      param.offset_ref() = 135;
      param.page_ref() = 0x11;
    }
    qsfp->writeTransceiver(param, fault);
  };
  // Store the original refresh interval and then change it to 0 so that we can
  // trigger a forced refresh
  std::string originalRefreshInterval;
  gflags::GetCommandLineOption(
      "qsfp_data_refresh_interval", &originalRefreshInterval);
  gflags::SetCommandLineOptionWithMode(
      "qsfp_data_refresh_interval", "0", gflags::SET_FLAGS_DEFAULT);

  // Clear the cached media signals initially
  qsfp->readAndClearCachedMediaLaneSignals();
  qsfp->refresh();

  // Mark Tx Faults as false
  writeTxFault(0);
  qsfp->refresh();

  // Read the cached signals twice(in case tx fault was originally asserted in
  // the eeprom), it should return false for all lanes
  auto cachedSignal = qsfp->readAndClearCachedMediaLaneSignals();
  cachedSignal = qsfp->readAndClearCachedMediaLaneSignals();
  EXPECT_EQ(cachedSignal.size(), qsfp->numMediaLanes());
  for (const auto& kv : cachedSignal) {
    EXPECT_EQ(kv.second.txFault_ref().value_or({}), false);
  }
  cachedSignal.clear();

  // Set TX Faults on all lanes and then clear it. We want to make sure the
  // current tx fault status returns false, but the cached fault returns true.
  writeTxFault(0xFF);
  qsfp->refresh();
  writeTxFault(0);
  qsfp->refresh();

  // Read the cached tx fault, it should return true for all lanes
  cachedSignal = qsfp->readAndClearCachedMediaLaneSignals();
  EXPECT_EQ(cachedSignal.size(), qsfp->numMediaLanes());
  for (const auto& kv : cachedSignal) {
    EXPECT_EQ(kv.second.txFault_ref().value_or({}), true);
  }
  // Read the current tx fault, it should return false for all lanes
  TransceiverInfo info = qsfp->getTransceiverInfo();
  for (const auto& signal : info.mediaLaneSignals_ref().value_or({})) {
    EXPECT_EQ(signal.txFault_ref().value_or({}), false);
  }
  cachedSignal.clear();

  // Read the cache again, fault should go back to false
  cachedSignal = qsfp->readAndClearCachedMediaLaneSignals();
  EXPECT_EQ(cachedSignal.size(), qsfp->numMediaLanes());
  for (const auto& kv : cachedSignal) {
    EXPECT_EQ(kv.second.txFault_ref().value_or({}), false);
  }

  gflags::SetCommandLineOptionWithMode(
      "qsfp_data_refresh_interval",
      originalRefreshInterval.c_str(),
      gflags::SET_FLAGS_DEFAULT);
}

void TransceiverTestsHelper::verifyVendorName(const std::string& expected) {
  EXPECT_EQ(expected, *info_.vendor_ref().value_or({}).name_ref());
}

void TransceiverTestsHelper::verifyTemp(double temp) {
  EXPECT_DOUBLE_EQ(
      temp, *info_.sensor_ref().value_or({}).temp_ref()->value_ref());
}

void TransceiverTestsHelper::verifyVcc(double vcc) {
  EXPECT_DOUBLE_EQ(
      vcc, *info_.sensor_ref().value_or({}).vcc_ref()->value_ref());
}

void TransceiverTestsHelper::verifyLaneDom(
    std::map<std::string, std::vector<double>>& laneDom,
    int lanes) {
  EXPECT_EQ(lanes, (*info_.channels_ref()).size());
  for (auto& channel : *info_.channels_ref()) {
    auto txPwr = *channel.sensors_ref()->txPwr_ref();
    auto rxPwr = *channel.sensors_ref()->rxPwr_ref();
    auto txBias = *channel.sensors_ref()->txBias_ref();
    EXPECT_DOUBLE_EQ(
        laneDom["TxBias"][*channel.channel_ref()], *(txBias.value_ref()));
    EXPECT_DOUBLE_EQ(
        laneDom["TxPwr"][*channel.channel_ref()], *(txPwr.value_ref()));
    EXPECT_DOUBLE_EQ(
        laneDom["RxPwr"][*channel.channel_ref()], *(rxPwr.value_ref()));
  }
}

void TransceiverTestsHelper::verifyMediaLaneSignals(
    std::map<std::string, std::vector<bool>>& expectedMediaSignals,
    int lanes) {
  EXPECT_EQ(lanes, info_.mediaLaneSignals_ref().value_or({}).size());
  for (auto& signal : info_.mediaLaneSignals_ref().value_or({})) {
    if (expectedMediaSignals.find("Tx_Los") != expectedMediaSignals.end()) {
      EXPECT_EQ(
          expectedMediaSignals["Tx_Los"][*signal.lane_ref()],
          signal.txLos_ref().value_or({}));
    }
    if (expectedMediaSignals.find("Rx_Los") != expectedMediaSignals.end()) {
      EXPECT_EQ(
          expectedMediaSignals["Rx_Los"][*signal.lane_ref()],
          signal.rxLos_ref().value_or({}));
    }
    if (expectedMediaSignals.find("Tx_Lol") != expectedMediaSignals.end()) {
      EXPECT_EQ(
          expectedMediaSignals["Tx_Lol"][*signal.lane_ref()],
          signal.txLol_ref().value_or({}));
    }
    if (expectedMediaSignals.find("Rx_Lol") != expectedMediaSignals.end()) {
      EXPECT_EQ(
          expectedMediaSignals["Rx_Lol"][*signal.lane_ref()],
          signal.rxLol_ref().value_or({}));
    }
    if (expectedMediaSignals.find("Tx_Fault") != expectedMediaSignals.end()) {
      EXPECT_EQ(
          expectedMediaSignals["Tx_Fault"][*signal.lane_ref()],
          signal.txFault_ref().value_or({}));
    }
    if (expectedMediaSignals.find("Tx_AdaptFault") !=
        expectedMediaSignals.end()) {
      EXPECT_EQ(
          expectedMediaSignals["Tx_AdaptFault"][*signal.lane_ref()],
          signal.txAdaptEqFault_ref().value_or({}));
    }
  }
}

void TransceiverTestsHelper::verifyMediaLaneSettings(
    std::map<std::string, std::vector<bool>>& expectedLaneSettings,
    int lanes) {
  auto settings = info_.settings_ref().value_or({});
  EXPECT_EQ(lanes, settings.mediaLaneSettings_ref().value_or({}).size());
  for (auto& setting : settings.mediaLaneSettings_ref().value_or({})) {
    if (expectedLaneSettings.find("TxDisable") != expectedLaneSettings.end()) {
      EXPECT_EQ(
          expectedLaneSettings["TxDisable"][*setting.lane_ref()],
          setting.txDisable_ref().value_or({}));
    }
    if (expectedLaneSettings.find("TxSqDisable") !=
        expectedLaneSettings.end()) {
      EXPECT_EQ(
          expectedLaneSettings["TxSqDisable"][*setting.lane_ref()],
          setting.txSquelch_ref().value_or({}));
    }
    if (expectedLaneSettings.find("TxForcedSq") != expectedLaneSettings.end()) {
      EXPECT_EQ(
          expectedLaneSettings["TxForcedSq"][*setting.lane_ref()],
          setting.txSquelchForce_ref().value_or({}));
    }
    if (expectedLaneSettings.find("TxAdaptEqCtrl") !=
        expectedLaneSettings.end()) {
      EXPECT_EQ(
          expectedLaneSettings["TxAdaptEqCtrl"][*setting.lane_ref()],
          setting.txAdaptiveEqControl_ref().value_or({}));
    }
  }
}

void TransceiverTestsHelper::verifyHostLaneSettings(
    std::map<std::string, std::vector<uint8_t>>& expectedLaneSettings,
    int lanes) {
  auto settings = info_.settings_ref().value_or({});
  EXPECT_EQ(lanes, settings.hostLaneSettings_ref().value_or({}).size());
  for (auto& setting : settings.hostLaneSettings_ref().value_or({})) {
    if (expectedLaneSettings.find("RxOutDisable") !=
        expectedLaneSettings.end()) {
      EXPECT_EQ(
          expectedLaneSettings["RxOutDisable"][*setting.lane_ref()],
          setting.rxOutput_ref().value_or({}));
    }
    if (expectedLaneSettings.find("RxSqDisable") !=
        expectedLaneSettings.end()) {
      EXPECT_EQ(
          expectedLaneSettings["RxSqDisable"][*setting.lane_ref()],
          setting.rxSquelch_ref().value_or({}));
    }
    if (expectedLaneSettings.find("RxEqPrecursor") !=
        expectedLaneSettings.end()) {
      EXPECT_EQ(
          expectedLaneSettings["RxEqPrecursor"][*setting.lane_ref()],
          setting.rxOutputPreCursor_ref().value_or({}));
    }
    if (expectedLaneSettings.find("RxEqMain") != expectedLaneSettings.end()) {
      EXPECT_EQ(
          expectedLaneSettings["RxEqMain"][*setting.lane_ref()],
          setting.rxOutputAmplitude_ref().value_or({}));
    }
    if (expectedLaneSettings.find("RxEqPostcursor") !=
        expectedLaneSettings.end()) {
      EXPECT_EQ(
          expectedLaneSettings["RxEqPostcursor"][*setting.lane_ref()],
          setting.rxOutputPostCursor_ref().value_or({}));
    }
  }
}

void TransceiverTestsHelper::verifyLaneInterrupts(
    std::map<std::string, std::vector<bool>>& laneInterrupts,
    int lanes) {
  EXPECT_EQ(lanes, (*info_.channels_ref()).size());
  for (auto& channel : *info_.channels_ref()) {
    auto txPwr = *channel.sensors_ref()->txPwr_ref();
    auto txPwrFlag = txPwr.flags_ref().value_or({});
    auto rxPwr = *channel.sensors_ref()->rxPwr_ref();
    auto rxPwrFlag = rxPwr.flags_ref().value_or({});
    auto txBias = *channel.sensors_ref()->txBias_ref();
    auto txBiasFlag = txBias.flags_ref().value_or({});

    EXPECT_EQ(
        laneInterrupts["TxPwrHighAlarm"][*channel.channel_ref()],
        *(txPwrFlag.alarm_ref()->high_ref()));
    EXPECT_EQ(
        laneInterrupts["TxPwrHighWarn"][*channel.channel_ref()],
        *(txPwrFlag.warn_ref()->high_ref()));
    EXPECT_EQ(
        laneInterrupts["TxPwrLowAlarm"][*channel.channel_ref()],
        *(txPwrFlag.alarm_ref()->low_ref()));
    EXPECT_EQ(
        laneInterrupts["TxPwrLowWarn"][*channel.channel_ref()],
        *(txPwrFlag.warn_ref()->low_ref()));

    EXPECT_EQ(
        laneInterrupts["RxPwrHighAlarm"][*channel.channel_ref()],
        *(rxPwrFlag.alarm_ref()->high_ref()));
    EXPECT_EQ(
        laneInterrupts["RxPwrHighWarn"][*channel.channel_ref()],
        *(rxPwrFlag.warn_ref()->high_ref()));
    EXPECT_EQ(
        laneInterrupts["RxPwrLowAlarm"][*channel.channel_ref()],
        *(rxPwrFlag.alarm_ref()->low_ref()));
    EXPECT_EQ(
        laneInterrupts["RxPwrLowWarn"][*channel.channel_ref()],
        *(rxPwrFlag.warn_ref()->low_ref()));

    EXPECT_EQ(
        laneInterrupts["TxBiasHighAlarm"][*channel.channel_ref()],
        *(txBiasFlag.alarm_ref()->high_ref()));
    EXPECT_EQ(
        laneInterrupts["TxBiasHighWarn"][*channel.channel_ref()],
        *(txBiasFlag.warn_ref()->high_ref()));
    EXPECT_EQ(
        laneInterrupts["TxBiasLowAlarm"][*channel.channel_ref()],
        *(txBiasFlag.alarm_ref()->low_ref()));
    EXPECT_EQ(
        laneInterrupts["TxBiasLowWarn"][*channel.channel_ref()],
        *(txBiasFlag.warn_ref()->low_ref()));
  }
}

void TransceiverTestsHelper::verifyGlobalInterrupts(
    const std::string& sensor,
    bool highAlarm,
    bool lowAlarm,
    bool highWarn,
    bool lowWarn) {
  FlagLevels flag;
  if (sensor == "temp") {
    flag = info_.sensor_ref().value_or({}).temp_ref()->flags_ref().value_or({});
  } else {
    EXPECT_EQ(sensor, "vcc");
    flag = info_.sensor_ref().value_or({}).vcc_ref()->flags_ref().value_or({});
  }
  EXPECT_EQ(highAlarm, *flag.alarm_ref()->high_ref());
  EXPECT_EQ(highWarn, *flag.warn_ref()->high_ref());
  EXPECT_EQ(lowAlarm, *flag.alarm_ref()->low_ref());
  EXPECT_EQ(lowWarn, *flag.warn_ref()->low_ref());
}

void TransceiverTestsHelper::verifyThresholds(
    const std::string& sensor,
    double highAlarm,
    double lowAlarm,
    double highWarn,
    double lowWarn) {
  ThresholdLevels level;
  if (sensor == "temp") {
    level = *info_.thresholds_ref().value_or({}).temp_ref();
  } else if (sensor == "vcc") {
    level = *info_.thresholds_ref().value_or({}).vcc_ref();
  } else if (sensor == "txPwr") {
    level = *info_.thresholds_ref().value_or({}).txPwr_ref();
  } else if (sensor == "rxPwr") {
    level = *info_.thresholds_ref().value_or({}).rxPwr_ref();
  } else {
    level = *info_.thresholds_ref().value_or({}).txBias_ref();
  }
  EXPECT_DOUBLE_EQ(highAlarm, *level.alarm_ref()->high_ref());
  EXPECT_DOUBLE_EQ(highWarn, *level.warn_ref()->high_ref());
  EXPECT_DOUBLE_EQ(lowAlarm, *level.alarm_ref()->low_ref());
  EXPECT_DOUBLE_EQ(lowWarn, *level.warn_ref()->low_ref());
}

void TransceiverTestsHelper::verifyFwInfo(
    const std::string& expectedFwRev,
    const std::string& expectedDspFw,
    const std::string& expecedBuildRev) {
  EXPECT_EQ(
      expectedFwRev,
      *info_.status_ref()
           .value_or({})
           .fwStatus_ref()
           .value_or({})
           .version_ref());
  EXPECT_EQ(
      expectedDspFw,
      *info_.status_ref()
           .value_or({})
           .fwStatus_ref()
           .value_or({})
           .dspFwVer_ref());
  EXPECT_EQ(
      expecedBuildRev,
      *info_.status_ref()
           .value_or({})
           .fwStatus_ref()
           .value_or({})
           .buildRev_ref());
}

} // namespace fboss
} // namespace facebook
