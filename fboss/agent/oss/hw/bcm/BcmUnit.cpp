/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmUnit.h"

#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/bcm/BcmError.h"

extern "C" {
#include <opennsl/init.h>
#include <opennsl/switch.h>
} // extern "C"

using folly::StringPiece;
using facebook::fboss::BcmAPI;

namespace facebook { namespace fboss {

BcmUnit::BcmUnit(int deviceIndex) {
  // For now we assume that the unit number is 0. This will be changed once
  // opennsl exposes interfaces to determine which units are on the system
  unit_ = 0;
}

BcmUnit::~BcmUnit() {
  if (attached_.load(std::memory_order_acquire)) {
    auto rv = opennsl_detach(unit_);
    CHECK(OPENNSL_SUCCESS(rv)) <<
        "failed to detach BCM unit " << unit_ << ": " << opennsl_errmsg(rv);
  }
  // Unregister ourselves from BcmAPI.
  BcmAPI::unitDestroyed(this);
}

void BcmUnit::detach() {
  attached_.store(false, std::memory_order_release);

  // Clean up SDK state, without touching the hardware
  auto rv = _opennsl_shutdown(unit_);
  bcmCheckError(rv, "failed to clean up BCM state during warm boot shutdown");
}

void BcmUnit::attach() {
  if (attached_.load(std::memory_order_acquire)) {
    throw FbossError("unit ", unit_, " already initialized");
  }

  auto rv = opennsl_switch_event_register(unit_, switchEventCallback, this);
  bcmCheckError(rv, "unable to register switch event callback for unit ",
                unit_);

  attached_.store(true, std::memory_order_release);
}

void BcmUnit::onSwitchEvent(opennsl_switch_event_t event,
                            uint32_t arg1, uint32_t arg2, uint32_t arg3) {
  switch (event) {
    case OPENNSL_SWITCH_EVENT_STABLE_FULL:
    case OPENNSL_SWITCH_EVENT_STABLE_ERROR:
    case OPENNSL_SWITCH_EVENT_UNCONTROLLED_SHUTDOWN:
    case OPENNSL_SWITCH_EVENT_WARM_BOOT_DOWNGRADE:
      LOG(FATAL) << "Fatal switch event : " << event << "occured";
      break;
    default:
      break;
  }
}

void BcmUnit::switchEventCallback(int unitNumber,
                                  opennsl_switch_event_t event,
                                  uint32_t arg1,
                                  uint32_t arg2,
                                  uint32_t arg3,
                                  void* userdata) {
  BcmUnit* unit = reinterpret_cast<BcmUnit*>(userdata);
  try {
    unit->onSwitchEvent(event, arg1, arg2, arg3);
  } catch (const std::exception& ex) {
    LOG(FATAL) << "unhandled exception while processing switch event "
      << event << "(" << arg1 << ", " << arg2 << ", " << arg3
      << ") on unit " << unitNumber;
  }
}

}} // facebook::fboss
