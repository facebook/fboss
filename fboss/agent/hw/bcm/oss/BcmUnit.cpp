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
#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"

extern "C" {
#include <opennsl/init.h>
#include <opennsl/switch.h>
#include <sal/driver.h>
} // extern "C"

using folly::StringPiece;
using facebook::fboss::BcmAPI;

namespace facebook { namespace fboss {

namespace {
constexpr auto wbEnvVar = "SOC_BOOT_FLAGS";
constexpr auto wbFlag = "0x200000";
}


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

void BcmUnit::detach(const std::string& switchStateFile,
    folly::dynamic& switchState) {
  attached_.store(false, std::memory_order_release);

  // Clean up SDK state, without touching the hardware
  auto rv = _opennsl_shutdown(unit_);
  bcmCheckError(rv, "failed to clean up BCM state during warm boot shutdown");

  wbHelper_->setCanWarmBoot();
}

void BcmUnit::attach(std::string warmBootDir) {
  if (attached_.load(std::memory_order_acquire)) {
    throw FbossError("unit ", unit_, " already initialized");
  }

  /*
   * FIXME(aeckert): unsetenv and setenv are not thread safe. This will
   * be called after the thrift thread has already started so this is
   * not safe to do. We need to fix this once broadcom provides a better
   * API for setting the boot flags.
   *
   * FIXME(orib): This assumes that we only ever have one unit set up,
   * and that as a result, opennsl_driver_init() will only be called once.
   */
  wbHelper_ = folly::make_unique<BcmWarmBootHelper>(unit_, warmBootDir);
  unsetenv(wbEnvVar);
  if(warmBootHelper()->canWarmBoot()) {
    setenv(wbEnvVar, wbFlag, 1);
  }

  opennsl_driver_init();
  auto rv = opennsl_switch_event_register(unit_, switchEventCallback, this);
  bcmCheckError(rv, "unable to register switch event callback for unit ",
                unit_);

  attached_.store(true, std::memory_order_release);
}

void BcmUnit::attach() {
  attach("");
}

BootType BcmUnit::bootType() {
  // TODO(aeckert): Move this function into fboss/agent/hw/bcm/BcmUnit.cpp
  if (!attached_.load(std::memory_order_acquire)) {
    return BootType::UNINITIALIZED;
  }

  return wbHelper_->canWarmBoot() ? BootType::WARM_BOOT : BootType::COLD_BOOT;
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

void BcmUnit::rawRegisterWrite(uint16_t phyID,
                               uint8_t reg,
                               uint16_t data) {
  // stubbed out
}

}} // facebook::fboss
