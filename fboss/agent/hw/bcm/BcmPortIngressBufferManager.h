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
  PortPgConfigs getCurrentProgrammedPgSettingsHw();
  PortPgConfigs getCurrentPgSettingsHw();
  static const PortPgConfig& getDefaultChipPgSettings(utility::BcmChip chip);
  const PortPgConfig& getDefaultPgSettings();

  // used for test utils
  PgIdSet getPgIdListInHw();
  void setPgIdListInHw(PgIdSet& newPgIdList);

 private:
  /* read APIs to the BCM SDK */
  void getIngressAlpha(
      bcm_cos_queue_t cosq,
      std::shared_ptr<PortPgConfig> pgConfig);
  void getPgMinLimitBytes(
      bcm_cos_queue_t cosQ,
      std::shared_ptr<PortPgConfig> pgConfig);
  void getPgResumeOffsetBytes(
      bcm_cos_queue_t cosQ,
      std::shared_ptr<PortPgConfig> pgConfig);
  void getPgHeadroomLimitBytes(
      bcm_cos_queue_t cosQ,
      std::shared_ptr<PortPgConfig> pgConfig);
  void programPg(const PortPgConfig* pgConfig, const int cosq);
  void resetPgToDefault(int pgId);
  void resetPgsToDefault();
  void reprogramPgs(const std::shared_ptr<Port> port);
  void writeCosqTypeToHw(
      const int cosq,
      const bcm_cosq_control_t type,
      const int value,
      const std::string& typeStr);
  void readCosqTypeFromHw(
      const int cosq,
      const bcm_cosq_control_t type,
      int* value,
      const std::string& typeStr);
  void getPgParamsHw(const int pgId, const std::shared_ptr<PortPgConfig>& pg);

  BcmSwitch* hw_;
  std::string portName_;
  bcm_gport_t gport_;
  int unit_{-1};
  PgIdSet pgIdListInHw_;
  std::mutex pgIdListLock_;
};

} // namespace facebook::fboss
