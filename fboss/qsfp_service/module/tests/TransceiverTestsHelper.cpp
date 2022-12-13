// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/module/tests/TransceiverTestsHelper.h"

#include <gtest/gtest.h>

namespace facebook {
namespace fboss {

void testCachedMediaSignals(QsfpModule* qsfp) {
  auto mgmtInterface =
      qsfp->getTransceiverInfo().transceiverManagementInterface().value_or({});
  auto writeTxFault = [&](uint8_t fault) {
    TransceiverIOParameters param;
    param.length() = 1;
    if (mgmtInterface == TransceiverManagementInterface::SFF) {
      param.offset() = 4;
      param.page() = 0;
    } else {
      param.offset() = 135;
      param.page() = 0x11;
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
    EXPECT_EQ(kv.second.txFault().value_or({}), false);
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
    EXPECT_EQ(kv.second.txFault().value_or({}), true);
  }
  // Read the current tx fault, it should return false for all lanes
  TransceiverInfo info = qsfp->getTransceiverInfo();
  for (const auto& signal : info.mediaLaneSignals().value_or({})) {
    EXPECT_EQ(signal.txFault().value_or({}), false);
  }
  cachedSignal.clear();

  // Read the cache again, fault should go back to false
  cachedSignal = qsfp->readAndClearCachedMediaLaneSignals();
  EXPECT_EQ(cachedSignal.size(), qsfp->numMediaLanes());
  for (const auto& kv : cachedSignal) {
    EXPECT_EQ(kv.second.txFault().value_or({}), false);
  }

  gflags::SetCommandLineOptionWithMode(
      "qsfp_data_refresh_interval",
      originalRefreshInterval.c_str(),
      gflags::SET_FLAGS_DEFAULT);
}

void TransceiverTestsHelper::verifyPrbsPolynomials(
    std::vector<prbs::PrbsPolynomial>& expectedPolynomials,
    std::vector<prbs::PrbsPolynomial>& foundPolynomials) {
  std::sort(expectedPolynomials.begin(), expectedPolynomials.end());
  std::sort(foundPolynomials.begin(), foundPolynomials.end());
  EXPECT_EQ(expectedPolynomials, foundPolynomials);
}

void TransceiverTestsHelper::verifyVendorName(const std::string& expected) {
  EXPECT_EQ(expected, *info_.vendor().value_or({}).name());
}

void TransceiverTestsHelper::verifyTemp(double temp) {
  EXPECT_DOUBLE_EQ(temp, *info_.sensor().value_or({}).temp()->value());
}

void TransceiverTestsHelper::verifyVcc(double vcc) {
  EXPECT_DOUBLE_EQ(vcc, *info_.sensor().value_or({}).vcc()->value());
}

void TransceiverTestsHelper::verifyLaneDom(
    std::map<std::string, std::vector<double>>& laneDom,
    int lanes) {
  EXPECT_EQ(lanes, (*info_.channels()).size());
  for (auto& channel : *info_.channels()) {
    auto txPwr = *channel.sensors()->txPwr();
    auto rxPwr = *channel.sensors()->rxPwr();
    auto txBias = *channel.sensors()->txBias();
    EXPECT_DOUBLE_EQ(laneDom["TxBias"][*channel.channel()], *(txBias.value()));
    EXPECT_DOUBLE_EQ(laneDom["TxPwr"][*channel.channel()], *(txPwr.value()));
    EXPECT_DOUBLE_EQ(laneDom["RxPwr"][*channel.channel()], *(rxPwr.value()));
  }
}

void TransceiverTestsHelper::verifyLaneSignals(
    std::map<std::string, std::vector<bool>>& expectedSignals,
    int numHostLanes,
    int numMediaLanes) {
  verifyHostLaneSignals(expectedSignals, numHostLanes);
  verifyMediaLaneSignals(expectedSignals, numMediaLanes);
}

void TransceiverTestsHelper::verifyMediaLaneSignals(
    std::map<std::string, std::vector<bool>>& expectedMediaSignals,
    int lanes) {
  EXPECT_EQ(lanes, info_.mediaLaneSignals().value_or({}).size());
  for (auto& signal : info_.mediaLaneSignals().value_or({})) {
    if (expectedMediaSignals.find("Rx_Los") != expectedMediaSignals.end()) {
      EXPECT_EQ(
          expectedMediaSignals["Rx_Los"][*signal.lane()],
          signal.rxLos().value_or({}));
    }
    if (expectedMediaSignals.find("Rx_Lol") != expectedMediaSignals.end()) {
      EXPECT_EQ(
          expectedMediaSignals["Rx_Lol"][*signal.lane()],
          signal.rxLol().value_or({}));
    }
    if (expectedMediaSignals.find("Tx_Fault") != expectedMediaSignals.end()) {
      EXPECT_EQ(
          expectedMediaSignals["Tx_Fault"][*signal.lane()],
          signal.txFault().value_or({}));
    }
  }
}

void TransceiverTestsHelper::verifyHostLaneSignals(
    std::map<std::string, std::vector<bool>>& expectedHostSignals,
    int lanes) {
  EXPECT_EQ(lanes, info_.hostLaneSignals().value_or({}).size());
  for (auto& signal : info_.hostLaneSignals().value_or({})) {
    if (expectedHostSignals.find("Tx_Los") != expectedHostSignals.end()) {
      EXPECT_EQ(
          expectedHostSignals["Tx_Los"][*signal.lane()],
          signal.txLos().value_or({}));
    }
    if (expectedHostSignals.find("Tx_Lol") != expectedHostSignals.end()) {
      EXPECT_EQ(
          expectedHostSignals["Tx_Lol"][*signal.lane()],
          signal.txLol().value_or({}));
    }
    if (expectedHostSignals.find("Tx_AdaptFault") !=
        expectedHostSignals.end()) {
      EXPECT_EQ(
          expectedHostSignals["Tx_AdaptFault"][*signal.lane()],
          signal.txAdaptEqFault().value_or({}));
    }
  }
}

void TransceiverTestsHelper::verifyMediaLaneSettings(
    std::map<std::string, std::vector<bool>>& expectedLaneSettings,
    int lanes) {
  auto settings = info_.settings().value_or({});
  EXPECT_EQ(lanes, settings.mediaLaneSettings().value_or({}).size());
  for (auto& setting : settings.mediaLaneSettings().value_or({})) {
    if (expectedLaneSettings.find("TxDisable") != expectedLaneSettings.end()) {
      EXPECT_EQ(
          expectedLaneSettings["TxDisable"][*setting.lane()],
          setting.txDisable().value_or({}));
    }
    if (expectedLaneSettings.find("TxSqDisable") !=
        expectedLaneSettings.end()) {
      EXPECT_EQ(
          expectedLaneSettings["TxSqDisable"][*setting.lane()],
          setting.txSquelch().value_or({}));
    }
    if (expectedLaneSettings.find("TxForcedSq") != expectedLaneSettings.end()) {
      EXPECT_EQ(
          expectedLaneSettings["TxForcedSq"][*setting.lane()],
          setting.txSquelchForce().value_or({}));
    }
    if (expectedLaneSettings.find("TxAdaptEqCtrl") !=
        expectedLaneSettings.end()) {
      EXPECT_EQ(
          expectedLaneSettings["TxAdaptEqCtrl"][*setting.lane()],
          setting.txAdaptiveEqControl().value_or({}));
    }
  }
}

void TransceiverTestsHelper::verifyHostLaneSettings(
    std::map<std::string, std::vector<uint8_t>>& expectedLaneSettings,
    int lanes) {
  auto settings = info_.settings().value_or({});
  EXPECT_EQ(lanes, settings.hostLaneSettings().value_or({}).size());
  for (auto& setting : settings.hostLaneSettings().value_or({})) {
    if (expectedLaneSettings.find("RxOutDisable") !=
        expectedLaneSettings.end()) {
      EXPECT_EQ(
          expectedLaneSettings["RxOutDisable"][*setting.lane()],
          setting.rxOutput().value_or({}));
    }
    if (expectedLaneSettings.find("RxSqDisable") !=
        expectedLaneSettings.end()) {
      EXPECT_EQ(
          expectedLaneSettings["RxSqDisable"][*setting.lane()],
          setting.rxSquelch().value_or({}));
    }
    if (expectedLaneSettings.find("RxEqPrecursor") !=
        expectedLaneSettings.end()) {
      EXPECT_EQ(
          expectedLaneSettings["RxEqPrecursor"][*setting.lane()],
          setting.rxOutputPreCursor().value_or({}));
    }
    if (expectedLaneSettings.find("RxEqMain") != expectedLaneSettings.end()) {
      EXPECT_EQ(
          expectedLaneSettings["RxEqMain"][*setting.lane()],
          setting.rxOutputAmplitude().value_or({}));
    }
    if (expectedLaneSettings.find("RxEqPostcursor") !=
        expectedLaneSettings.end()) {
      EXPECT_EQ(
          expectedLaneSettings["RxEqPostcursor"][*setting.lane()],
          setting.rxOutputPostCursor().value_or({}));
    }
  }
}

void TransceiverTestsHelper::verifyLaneInterrupts(
    std::map<std::string, std::vector<bool>>& laneInterrupts,
    int lanes) {
  EXPECT_EQ(lanes, (*info_.channels()).size());
  for (auto& channel : *info_.channels()) {
    auto txPwr = *channel.sensors()->txPwr();
    auto txPwrFlag = txPwr.flags().value_or({});
    auto rxPwr = *channel.sensors()->rxPwr();
    auto rxPwrFlag = rxPwr.flags().value_or({});
    auto txBias = *channel.sensors()->txBias();
    auto txBiasFlag = txBias.flags().value_or({});

    EXPECT_EQ(
        laneInterrupts["TxPwrHighAlarm"][*channel.channel()],
        *(txPwrFlag.alarm()->high()));
    EXPECT_EQ(
        laneInterrupts["TxPwrHighWarn"][*channel.channel()],
        *(txPwrFlag.warn()->high()));
    EXPECT_EQ(
        laneInterrupts["TxPwrLowAlarm"][*channel.channel()],
        *(txPwrFlag.alarm()->low()));
    EXPECT_EQ(
        laneInterrupts["TxPwrLowWarn"][*channel.channel()],
        *(txPwrFlag.warn()->low()));

    EXPECT_EQ(
        laneInterrupts["RxPwrHighAlarm"][*channel.channel()],
        *(rxPwrFlag.alarm()->high()));
    EXPECT_EQ(
        laneInterrupts["RxPwrHighWarn"][*channel.channel()],
        *(rxPwrFlag.warn()->high()));
    EXPECT_EQ(
        laneInterrupts["RxPwrLowAlarm"][*channel.channel()],
        *(rxPwrFlag.alarm()->low()));
    EXPECT_EQ(
        laneInterrupts["RxPwrLowWarn"][*channel.channel()],
        *(rxPwrFlag.warn()->low()));

    EXPECT_EQ(
        laneInterrupts["TxBiasHighAlarm"][*channel.channel()],
        *(txBiasFlag.alarm()->high()));
    EXPECT_EQ(
        laneInterrupts["TxBiasHighWarn"][*channel.channel()],
        *(txBiasFlag.warn()->high()));
    EXPECT_EQ(
        laneInterrupts["TxBiasLowAlarm"][*channel.channel()],
        *(txBiasFlag.alarm()->low()));
    EXPECT_EQ(
        laneInterrupts["TxBiasLowWarn"][*channel.channel()],
        *(txBiasFlag.warn()->low()));
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
    flag = info_.sensor().value_or({}).temp()->flags().value_or({});
  } else {
    EXPECT_EQ(sensor, "vcc");
    flag = info_.sensor().value_or({}).vcc()->flags().value_or({});
  }
  EXPECT_EQ(highAlarm, *flag.alarm()->high());
  EXPECT_EQ(highWarn, *flag.warn()->high());
  EXPECT_EQ(lowAlarm, *flag.alarm()->low());
  EXPECT_EQ(lowWarn, *flag.warn()->low());
}

void TransceiverTestsHelper::verifyThresholds(
    const std::string& sensor,
    double highAlarm,
    double lowAlarm,
    double highWarn,
    double lowWarn) {
  ThresholdLevels level;
  if (sensor == "temp") {
    level = *info_.thresholds().value_or({}).temp();
  } else if (sensor == "vcc") {
    level = *info_.thresholds().value_or({}).vcc();
  } else if (sensor == "txPwr") {
    level = *info_.thresholds().value_or({}).txPwr();
  } else if (sensor == "rxPwr") {
    level = *info_.thresholds().value_or({}).rxPwr();
  } else {
    level = *info_.thresholds().value_or({}).txBias();
  }
  EXPECT_DOUBLE_EQ(highAlarm, *level.alarm()->high());
  EXPECT_DOUBLE_EQ(highWarn, *level.warn()->high());
  EXPECT_DOUBLE_EQ(lowAlarm, *level.alarm()->low());
  EXPECT_DOUBLE_EQ(lowWarn, *level.warn()->low());
}

void TransceiverTestsHelper::verifyFwInfo(
    const std::string& expectedFwRev,
    const std::string& expectedDspFw,
    const std::string& expecedBuildRev) {
  EXPECT_EQ(
      expectedFwRev,
      *info_.status().value_or({}).fwStatus().value_or({}).version());
  EXPECT_EQ(
      expectedDspFw,
      *info_.status().value_or({}).fwStatus().value_or({}).dspFwVer());
  EXPECT_EQ(
      expecedBuildRev,
      *info_.status().value_or({}).fwStatus().value_or({}).buildRev());
}

} // namespace fboss
} // namespace facebook
