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

#include "fboss/agent/hw/bcm/BcmPlatformPort.h"
#include "fboss/agent/gen-cpp/switch_config_types.h"
#include "fboss/agent/QsfpModule.h"

namespace facebook { namespace fboss {

class WedgePort : public BcmPlatformPort {
 protected:
  explicit WedgePort(PortID id);

 public:
  PortID getPortID() const override { return id_; }

  void setBcmPort(BcmPort* port) override;
  BcmPort* getBcmPort() const override {
    return bcmPort_;
  }

  void setQsfp(QsfpModule* qsfp) {
    qsfp_= qsfp;
  }

  void preDisable(bool temporary) override;
  void postDisable(bool temporary) override;
  void preEnable() override;
  void postEnable() override;
  bool isMediaPresent() override;
  void statusIndication(bool enabled, bool link,
                        bool ingress, bool egress,
                        bool discards, bool errors) override;
  void linkSpeedChanged(const cfg::PortSpeed& speed) override;
 private:
  // Forbidden copy constructor and assignment operator
  WedgePort(WedgePort const &) = delete;
  WedgePort& operator=(WedgePort const &) = delete;

 protected:
  PortID id_{0};
  BcmPort* bcmPort_{nullptr};
  // This is owned by SwSwitch
  QsfpModule* qsfp_{nullptr};
};

}} // facebook::fboss
