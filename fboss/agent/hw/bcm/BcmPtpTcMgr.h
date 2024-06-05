// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/executors/FunctionScheduler.h>
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss {

class BcmPtpTcMgr {
 public:
  explicit BcmPtpTcMgr(BcmSwitch* hw) : hw_(hw) {}
  ~BcmPtpTcMgr() {}

  void enablePtpTc();
  void disablePtpTc();

  // utility routines
  static bool isPtpTcSupported(const BcmSwitchIf* hw);
  static bool isPtpTcPcsSupported(const BcmSwitchIf* hw);
  static bool isPtpTcEnabled(const BcmSwitchIf* hw);

 private:
  static int getTsBitModeArg(cfg::AsicType asicType);

  void enablePortTimesyncConfig(
      bcm_port_timesync_config_t* port_timesync_config);

  void enablePcsBasedOneStepTimestamping(bcm_port_t port);
  void disablePcsBasedOneStepTimestamping(bcm_port_t port);
  static bool isPcsBasedOneStepTimestampingEnabled(
      const BcmSwitchIf* hw,
      bcm_port_t port);

  void enableTimeInterface();
  void disableTimeInterface();
  static bool isTimeInterfaceEnabled(const BcmSwitchIf* hw);

  BcmSwitch* hw_;
};

} // namespace facebook::fboss
