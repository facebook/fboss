// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/qsfp_service/module/QsfpModule.h"

namespace facebook {
namespace fboss {

class TransceiverTestsHelper {
 public:
  explicit TransceiverTestsHelper(TransceiverInfo& info) : info_(info) {}
  void verifyVendorName(const std::string& expected);
  void verifyTemp(double temp);
  void verifyVcc(double vcc);
  void verifyLaneDom(
      std::map<std::string, std::vector<double>>& laneDom,
      int lanes);
  void verifyMediaLaneSignals(
      std::map<std::string, std::vector<bool>>& expectedMediaSignals,
      int lanes);
  void verifyMediaLaneSettings(
      std::map<std::string, std::vector<bool>>& expectedLaneSettings,
      int lanes);
  void verifyHostLaneSettings(
      std::map<std::string, std::vector<uint8_t>>& expectedLaneSettings,
      int lanes);
  void verifyLaneInterrupts(
      std::map<std::string, std::vector<bool>>& laneInterrupts,
      int lanes);
  void verifyGlobalInterrupts(
      const std::string& sensor,
      bool highAlarm,
      bool lowAlarm,
      bool highWarn,
      bool lowWarn);
  void verifyThresholds(
      const std::string& sensor,
      double highAlarm,
      double lowAlarm,
      double highWarn,
      double lowWarn);
  void verifyFwInfo(
      const std::string& expectedFwRev,
      const std::string& expectedDspFw,
      const std::string& expecedBuildRev);

 private:
  TransceiverInfo info_;
};

void testCachedMediaSignals(QsfpModule* qsfp);

} // namespace fboss
} // namespace facebook
