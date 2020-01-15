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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmUnit.h"
#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"

#include <folly/Memory.h>
#include <folly/String.h>
#include <folly/io/Cursor.h>
#include <folly/logging/xlog.h>
#include <glog/logging.h>

#include <atomic>
#include <unordered_map>

using folly::StringPiece;
using std::make_unique;
using std::string;

DEFINE_string(
    l2xmsg_mode,
    "1",
    "Deliver L2 learning update callback via interrupt,"
    "drain L2 Mod FIFO on delivering callback");

/*
 * bde_create() must be defined as a symbol when linking against BRCM libs.
 * It should never be invoked in our setup though. So return a error
 */
extern "C" int bde_create() {
  XLOG(ERR) << "unexpected call to bde_create(): probe invoked "
               "via diag shell command?";
  return BCM_E_UNAVAIL;
}
/*
 * We don't set any default values.
 */
extern "C" void sal_config_init_defaults() {}

namespace {

/*
 * TODO (skhare)
 * Configerator change D18746658 introduces l2xmsg_mode to BCM config. It would
 * be activated as part of next disruptive upgrade which could take of the
 * order of several months/a year.
 *
 * We need l2xmst_mode setting sooner: MH-NIC queue-per-host L2 fix requires
 * it. Thus, temporarily hard code l2xmsg mode here. We also provide
 * FLAGS_l2xmsg_mode to disable this if needed.
 *
 * Broadcom has explicitly confirmed that setting l2xmsg_mode is safe across
 * the warmboot, and we have BCM tests that verify it.
 *
 * This logic can be removed on a fleet wide disruptive upgrade after D18746658
 * lands.
 *
 * The map is explicitly named as kBcmConfigsSafeAcrossWarmboot as only BCM
 * configs that can be safely applied post warmboot could be added here as
 * temporary workaround.
 */
const std::map<StringPiece, std::string> kBcmConfigsSafeAcrossWarmboot = {
    /*
     * Configure to get the callback via interrupts. Default is polling mode
     * which is expensive as a thread must periodically poll for the L2 table
     * updates. It is particularly wasteful given that the L2 table would likely
     * not change that often.
     * L2 MOD FIFO is used to queue up callbacks. If l2xmsg_mode is set to 1,
     * the L2 MOD FIFO is dequeued whenever a callback is delivered, otherwise
     * L2 MOD FIFO gets built up.
     */
    {"l2xmsg_mode", FLAGS_l2xmsg_mode},
};

} // namespace

namespace facebook::fboss {

static std::atomic<bool> bcmInitialized{false};
static BcmAPI::HwConfigMap bcmConfig;

extern std::atomic<BcmUnit*> bcmUnits[];

std::unique_ptr<BcmAPI> BcmAPI::singleton_;

void BcmAPI::initConfig(const std::map<std::string, std::string>& config) {
  // Store the configuration settings
  bcmConfig.clear();
  for (const auto& entry : config) {
    bcmConfig.emplace(entry.first, entry.second);
  }
}

const char* FOLLY_NULLABLE BcmAPI::getConfigValue(StringPiece name) {
  auto it = bcmConfig.find(name);
  if (it != bcmConfig.end()) {
    return it->second.c_str();
  }

  /*
   * If a config is not part of bcmConfig, check the list of hard coded
   * configs, see comment at the top of kBcmConfigsSafeAcrossWarmboot for
   * additional context.
   */
  auto it2 = kBcmConfigsSafeAcrossWarmboot.find(name);
  if (it2 != kBcmConfigsSafeAcrossWarmboot.end()) {
    return it2->second.c_str();
  }

  return nullptr;
}

BcmAPI::HwConfigMap BcmAPI::getHwConfig() {
  return bcmConfig;
}

std::unique_ptr<BcmUnit> BcmAPI::createUnit(
    int deviceIndex,
    BcmPlatform* platform) {
  auto unitObj = make_unique<BcmUnit>(deviceIndex, platform);
  int unit = unitObj->getNumber();
  BcmUnit* expectedUnit{nullptr};
  if (!bcmUnits[unit].compare_exchange_strong(
          expectedUnit, unitObj.get(), std::memory_order_acq_rel)) {
    throw FbossError("a BcmUnit already exists for unit number ", unit);
  }
  platform->onUnitCreate(unit);

  return unitObj;
}

void BcmAPI::initUnit(int unit, BcmPlatform* platform) {
  BcmUnit* unitObj = getUnit(unit);
  if (platform->getWarmBootHelper()->canWarmBoot()) {
    unitObj->warmBootAttach();
  } else {
    unitObj->coldBootAttach();
  }
  platform->onUnitAttach(unit);
}

void BcmAPI::init(const std::map<std::string, std::string>& config) {
  if (bcmInitialized.load(std::memory_order_acquire)) {
    return;
  }

  initConfig(config);
  BcmAPI::initImpl();

  bcmInitialized.store(true, std::memory_order_release);
}

std::unique_ptr<BcmUnit> BcmAPI::createOnlyUnit(BcmPlatform* platform) {
  auto numDevices = BcmAPI::getNumSwitches();
  if (numDevices == 0) {
    throw FbossError("no Broadcom switching ASIC found");
  } else if (numDevices > 1) {
    throw FbossError("found more than 1 Broadcom switching ASIC");
  }
  return createUnit(0, platform);
}

void BcmAPI::unitDestroyed(BcmUnit* unit) {
  int num = unit->getNumber();
  BcmUnit* expectedUnit{unit};
  if (!bcmUnits[num].compare_exchange_strong(
          expectedUnit, nullptr, std::memory_order_acq_rel)) {
    XLOG(FATAL) << "inconsistency in BCM unit array for unit " << num
                << ": expected " << (void*)unit << " but found "
                << (void*)expectedUnit;
  }
  bcmInitialized.store(false, std::memory_order_release);
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

} // namespace facebook::fboss
