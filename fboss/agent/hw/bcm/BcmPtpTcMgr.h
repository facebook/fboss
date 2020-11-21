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

 private:
  bool isPtpTcSupported();
  bool isPtpTcPcsSupported();

  int getTsBitModeArg(HwAsic::AsicType asicType);

  void enablePortTimesyncConfig(
      bcm_port_timesync_config_t* port_timesync_config);

  void enablePcsBasedTimestamping(bcm_port_t port);
  void disablePcsBasedTimestamping(bcm_port_t port);

  void enableTimeInterface();
  void disableTimeInterface();

  BcmSwitch* hw_;
};

} // namespace facebook::fboss
