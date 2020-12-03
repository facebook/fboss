// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/experimental/FunctionScheduler.h>
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
  static bool isPtpTcSupported(BcmSwitch* hw);
  static bool isPtpTcPcsSupported(BcmSwitch* hw);

  // utility routines used by tests
  bool isPtpTcEnabled();

 private:
  int getTsBitModeArg(HwAsic::AsicType asicType);

  void enablePortTimesyncConfig(
      bcm_port_timesync_config_t* port_timesync_config);

  void enablePcsBasedOneStepTimestamping(bcm_port_t port);
  void disablePcsBasedOneStepTimestamping(bcm_port_t port);
  bool isPcsBasedOneStepTimestampingEnabled(bcm_port_t port);

  void enableTimeInterface();
  void disableTimeInterface();
  bool isTimeInterfaceEnabled();

  BcmSwitch* hw_;
};

} // namespace facebook::fboss
