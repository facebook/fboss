/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmAPI.h"

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmUnit.h"

#include <folly/Memory.h>
#include <folly/String.h>
#include <folly/io/Cursor.h>
#include <glog/logging.h>

#include <atomic>
#include <unordered_map>

using std::make_unique;
using folly::StringPiece;
using std::string;

namespace facebook { namespace fboss {

static std::atomic<bool> bcmInitialized{false};
extern std::atomic<BcmUnit*> bcmUnits[];

std::unique_ptr<BcmAPI> BcmAPI::singleton_;

std::unique_ptr<BcmUnit> BcmAPI::initUnit(int deviceIndex) {
  auto unitObj = make_unique<BcmUnit>(deviceIndex);
  int unit = unitObj->getNumber();
  BcmUnit* expectedUnit{nullptr};
  if (!bcmUnits[unit].compare_exchange_strong(expectedUnit, unitObj.get(),
                                              std::memory_order_acq_rel)) {
    throw FbossError("a BcmUnit already exists for unit number ", unit);
  }
  return unitObj;
}

void BcmAPI::init(const std::map<std::string, std::string>& config) {
    if (bcmInitialized.load(std::memory_order_acquire)) {
      return;
    }

    BcmAPI::initImpl(config);

    bcmInitialized.store(true, std::memory_order_release);
}


std::unique_ptr<BcmUnit> BcmAPI::initOnlyUnit() {
  auto numDevices = BcmAPI::getNumSwitches();
  if (numDevices == 0) {
    throw FbossError("no Broadcom switching ASIC found");
  } else if (numDevices > 1) {
    throw FbossError("found more than 1 Broadcom switching ASIC");
  }
  return initUnit(0);
}

void BcmAPI::unitDestroyed(BcmUnit* unit) {
  int num = unit->getNumber();
  BcmUnit* expectedUnit{unit};
  if (!bcmUnits[num].compare_exchange_strong(expectedUnit, nullptr,
                                             std::memory_order_acq_rel)) {
    LOG(FATAL) << "inconsistency in BCM unit array for unit " << num <<
      ": expected " << (void*)unit << " but found " << (void*)expectedUnit;
  }
}

BcmUnit* BcmAPI::getUnit(int unit) {
  if (unit < 0 || unit > getMaxSwitches()) {
    throw FbossError("invalid BCM unit number ", unit);
  }
  BcmUnit* unitObj = bcmUnits[unit].load(std::memory_order_acquire);
  if (!unitObj) {
    throw FbossError("no BcmUnit created for unit number ", unit);
  }
  return unitObj;
}

}} // facebook::fboss
