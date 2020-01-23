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

#include "fboss/agent/hw/test/HwLinkStateToggler.h"

#include <memory>

namespace facebook::fboss {

class SaiSwitch;
class Port;

class SaiLinkStateToggler : public HwLinkStateToggler {
 public:
  SaiLinkStateToggler(
      SaiSwitch* hw,
      StateUpdateFn stateUpdateFn,
      cfg::PortLoopbackMode desiredLoopbackMode)
      : HwLinkStateToggler(stateUpdateFn, desiredLoopbackMode), hw_(hw) {}

 private:
  void invokeLinkScanIfNeeded(PortID /*port*/, bool /*isUp*/) override {
    // TODO
  }
  void setPortPreemphasis(const std::shared_ptr<Port>& port, int preemphasis)
      override;

  SaiSwitch* hw_;
};

} // namespace facebook::fboss
