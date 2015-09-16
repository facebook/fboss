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

namespace facebook { namespace fboss {

class WedgePort : public BcmPlatformPort {
 public:
  explicit WedgePort(PortID id);

  PortID getPortID() const override { return id_; }

  void setBcmPort(BcmPort* port) override;
  BcmPort* getBcmPort() const override {
    return bcmPort_;
  }

  void preDisable(bool temporary) override;
  void postDisable(bool temporary) override;
  void preEnable() override;
  void postEnable() override;
  bool isMediaPresent() override;
  void linkStatusChanged(bool up, bool adminUp) override;
  void statusIndication(bool enabled, bool link,
                        bool ingress, bool egress,
                        bool discards, bool errors) override;

 private:
  // Forbidden copy constructor and assignment operator
  WedgePort(WedgePort const &) = delete;
  WedgePort& operator=(WedgePort const &) = delete;

  PortID id_{0};
  BcmPort* bcmPort_{nullptr};
};

}} // facebook::fboss
