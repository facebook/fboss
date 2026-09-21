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

#include "fboss/agent/platforms/sai/SaiBcmPlatformPort.h"
#include "fboss/lib/fpga/MinipackLed.h"

namespace facebook::fboss {

class SaiBcmMinipackPlatformPort : public SaiBcmPlatformPort,
                                   MultiPimPlatformPort {
 public:
  SaiBcmMinipackPlatformPort(PortID id, SaiPlatform* platform)
      : SaiBcmPlatformPort(id, platform),
        MultiPimPlatformPort(id, getPlatformPortEntry()) {}
  void linkStatusChanged(bool up, bool adminUp) override;
  void externalState(PortLedExternalState lfs) override;

 private:
  // (At least) two state machines, however trivial, run and affect LEDs.
  // - The normal internal admin/link state one.
  // - Whatever external things we support "intents" for.
  //
  // The external state takes precedence, while it's not NONE or until there
  // is an internal state change.  For the 2 external "intents" at present,
  // it is fine (and simple) for an internal link-state change to take
  // precedence and clear them, so the external state doesn't need tracking
  // (for now).
  //
  // Remember the LED state for internal, so that a transition of external
  // back to NONE can restore the internal LED state.
  MinipackLed::Color internalLedState_{MinipackLed::Color::OFF};
};

} // namespace facebook::fboss
