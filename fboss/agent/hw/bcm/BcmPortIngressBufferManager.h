/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

extern "C" {
#include <bcm/port.h>
}

#include <map>
#include <string>
#include <vector>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/BufferPoolConfig.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortPgConfig.h"
#include "fboss/agent/state/PortQueue.h"
// mostly for BcmChip
#include "fboss/agent/hw/bcm/BcmCosQueueManagerUtils.h"

namespace facebook::fboss {

class BcmSwitch;
using PgIdSet = std::set<int>;

class BcmPortIngressBufferManager {
 public:
  explicit BcmPortIngressBufferManager(
      BcmSwitch* hw,
      const std::string& portName,
      bcm_gport_t portGport);

  ~BcmPortIngressBufferManager() {}

  void programIngressBuffers(const std::shared_ptr<Port>& port);
  PortPgConfigs getCurrentProgrammedPgSettingsHw() const;
  PortPgConfigs getCurrentPgSettingsHw() const;
  BufferPoolCfgPtr getCurrentIngressPoolSettings() const;
  static const PortPgConfig& getDefaultChipPgSettings(utility::BcmChip chip);
  static const BufferPoolCfg& getDefaultChipIngressPoolSettings(
      utility::BcmChip chip);
  const PortPgConfig& getDefaultPgSettings() const;
  const BufferPoolCfg& getDefaultIngressPoolSettings() const;

  // used for test utils
  PgIdSet getPgIdListInHw() const;
  void setPgIdListInHw(PgIdSet& newPgIdList);
  int getProgrammedPgLosslessMode(const int pgId) const;
  int getProgrammedPfcStatusInPg(const int pgId) const;
  int getPgMinLimitBytes(const int pgId) const;
  int getIngressSharedBytes(bcm_cos_queue_t cosQ) const;

 private:
  /* read APIs to the BCM SDK */
  std::optional<cfg::MMUScalingFactor> getIngressAlpha(
      bcm_cos_queue_t cosq) const;
  int getPgResumeOffsetBytes(bcm_cos_queue_t cosQ) const;
  int getPgHeadroomLimitBytes(bcm_cos_queue_t cosQ) const;
  void programPg(state::PortPgFields pgConfig, const int cosq);
  void resetPgToDefault(int pgId);
  void resetIngressPoolsToDefault();
  void resetPgsToDefault(const std::shared_ptr<Port> port);
  void reprogramPgs(const std::shared_ptr<Port> port);
  void setIngressPoolHeadroomBytes(
      const bcm_cos_queue_t cosQ,
      const int headroomBytes);
  void setIngressSharedBytes(const bcm_cos_queue_t cosQ, const int sharedBytes);

  void writeCosqTypeToHw(
      const int cosq,
      const bcm_cosq_control_t type,
      const int value,
      const std::string& typeStr);
  void readCosqTypeFromHw(
      const int cosq,
      const bcm_cosq_control_t type,
      int* value,
      const std::string& typeStr) const;
  void getPgParamsHw(const int pgId, const std::shared_ptr<PortPgConfig>& pg)
      const;
  void reprogramIngressPools(const std::shared_ptr<Port> port);
  int getIngressPoolHeadroomBytes(bcm_cos_queue_t cosQ) const;
  void programLosslessMode(const std::shared_ptr<Port> port);
  void programPgLosslessMode(int pgId, int value);
  void programPfcOnPg(const int cosq, const int pfcEnable);
  void programPfcOnPgIfNeeded(const int cosq, const bool pfcEnable);
  void programPgLosslessModeIfNeeded(int pgId, int newPgLosslessMode);
  void writeCosqTypeToHwIfNeeded(
      const int cosq,
      const bcm_cosq_control_t type,
      const int value,
      const std::string& typeStr);

  BcmSwitch* hw_;
  std::string portName_;
  bcm_gport_t gport_;
  int unit_{-1};
  PgIdSet pgIdListInHw_;
  mutable std::mutex pgIdListLock_;
};

} // namespace facebook::fboss
