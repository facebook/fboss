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

#include "fboss/agent/Platform.h"
#include "fboss/agent/SysError.h"
#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmFwLoader.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmSdkVer.h"
#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

#include <folly/FileUtil.h>
#include <folly/ScopeGuard.h>
#include <folly/dynamic.h>
#include <folly/logging/xlog.h>
#include <glog/logging.h>

#include <chrono>

extern "C" {
#include <bcm/error.h>
#include <bcm/init.h>
#include <bcm/switch.h>
#include <ibde.h>
#include <soc/cmext.h>
#ifdef INCLUDE_PKTIO
#include <bcm/pktio.h>
#endif

#if (defined(IS_OPENNSA))
#define BCM_WARM_BOOT_SUPPORT
#include <soc/opensoc.h>
#else
#include <soc/cmic.h>
#include <soc/drv.h>
#endif
} // extern "C"

using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::steady_clock;

using facebook::fboss::BcmAPI;
using folly::StringPiece;

DEFINE_bool(use_pktio, false, "Select PKTIO as the slow_path TX/RX API");

DEFINE_string(
    script_pre_bcm_attach,
    "script_pre_bcm_attach",
    "Broadcom script file to be run after misc_init, before attach");

namespace {

/*
 * Device function callbacks.
 * In all of these callbacks, the BcmUnit* is available as dev->cookie.
 */

char* bcmGetConfigVar(soc_cm_dev_t* /*dev*/, const char* property) {
  // Get a configuration value for a unit.
  //
  // Note that while we could support separate configuration values for each
  // BcmUnit, the Broadcom SDK code generally assumes that the configuration is
  // shared across all units.
  //
  // When the SDK code wants to look up configuration property "foo", it will
  // check for the following configuration variables:
  //
  //   foo.<UNIT>
  //   foo.<UNIT_CHIP_NAME>
  //   foo.<UNIT_CHIP_GROUP>
  //   foo.<UNIT_FAMILY>  (if defined)
  //   foo
  //
  // (This does make lookup somewhat inefficient for configuration variables
  // that don't exist, since it has to check 4 or 5 names.)
  //
  // Given that the soc_property_get() code already supports unit-specific
  // variable name suffixes, there doesn't seem to be much benefit to
  // supporting per-unit configuration.
  const char* value = BcmAPI::getConfigValue(property);
  // The Broadcom SDK code won't really modify the returned value,
  // it just doesn't properly use const qualifiers.
  return const_cast<char*>(value);
}

int bdeInterruptConnect(
    soc_cm_dev_t* dev,
    soc_cm_isr_func_t handler,
    void* data) {
  return bde->interrupt_connect(dev->dev, handler, data);
}

int bdeInterruptDisconnect(soc_cm_dev_t* dev) {
  return bde->interrupt_disconnect(dev->dev);
}

uint32_t bdeRead(soc_cm_dev_t* dev, uint32_t addr) {
  return bde->read(dev->dev, addr);
}

void bdeWrite(soc_cm_dev_t* dev, uint32_t addr, uint32_t data) {
  bde->write(dev->dev, addr, data);
}

uint32_t bdePciConfRead(soc_cm_dev_t* dev, uint32_t addr) {
  return bde->pci_conf_read(dev->dev, addr);
}

void bdePciConfWrite(soc_cm_dev_t* dev, uint32_t addr, uint32_t data) {
  bde->pci_conf_write(dev->dev, addr, data);
}

void* bdeSalloc(soc_cm_dev_t* dev, int size, const char* name) {
  return bde->salloc(dev->dev, size, name);
}

void bdeSfree(soc_cm_dev_t* dev, void* ptr) {
  return bde->sfree(dev->dev, ptr);
}

int bdeSinval(soc_cm_dev_t* dev, void* addr, int length) {
  if (!bde->sinval) {
    return 0;
  }
  return bde->sinval(dev->dev, addr, length);
}

int bdeSflush(soc_cm_dev_t* dev, void* addr, int length) {
  if (!bde->sflush) {
    return 0;
  }
  return bde->sflush(dev->dev, addr, length);
}

sal_paddr_t bdeL2P_64(soc_cm_dev_t* dev, void* addr) {
  if (!bde->l2p) {
    return 0;
  }
  return bde->l2p(dev->dev, addr);
}

void* FOLLY_NULLABLE bdeP2L_64(soc_cm_dev_t* dev, sal_paddr_t addr) {
  if (!bde->p2l) {
    return nullptr;
  }
  return bde->p2l(dev->dev, addr);
}

uint32_t bdeIprocRead(soc_cm_dev_t* dev, uint32_t addr) {
  return bde->iproc_read(dev->dev, addr);
}

void bdeIprocWrite(soc_cm_dev_t* dev, uint32_t addr, uint32_t data) {
  bde->iproc_write(dev->dev, addr, data);
}

int bdeSpiRead(soc_cm_dev_t* dev, uint32_t addr, uint8_t* buf, int len) {
  if (!bde->spi_read) {
    return -1;
  }
  return bde->spi_read(dev->dev, addr, buf, len);
}

int bdeSpiWrite(soc_cm_dev_t* dev, uint32_t addr, uint8_t* buf, int len) {
  if (!bde->spi_write) {
    return -1;
  }
  return bde->spi_write(dev->dev, addr, buf, len);
}

} // unnamed namespace

namespace facebook::fboss {
void BcmUnit::writeWarmBootState(const folly::dynamic& follySwitchState) {
  if (!BcmAPI::isHwUsingHSDK()) {
    XLOG(DBG2) << " [Exit] Syncing BRCM switch state to file";
    steady_clock::time_point bcmWarmBootSyncStart = steady_clock::now();
    // Force the device to write out its warm boot state
    auto rv = bcm_switch_control_set(unit_, bcmSwitchControlSync, 1);
    bcmCheckError(rv, "Unable to sync state for L2 warm boot");

    steady_clock::time_point bcmWarmBootSyncDone = steady_clock::now();
    XLOG(DBG2) << "[Exit] BRCM warm boot sync time "
               << duration_cast<duration<float>>(
                      bcmWarmBootSyncDone - bcmWarmBootSyncStart)
                      .count();
  }
  // Now write our state to file
  XLOG(DBG2) << " [Exit] Syncing FBOSS switch state to file";
  steady_clock::time_point fbossWarmBootSyncStart = steady_clock::now();
  warmBootHelper()->storeHwSwitchWarmBootState(follySwitchState);
  steady_clock::time_point fbossWarmBootSyncDone = steady_clock::now();
  XLOG(DBG2) << "[Exit] Fboss warm boot sync time "
             << duration_cast<duration<float>>(
                    fbossWarmBootSyncDone - fbossWarmBootSyncStart)
                    .count();
}

void BcmUnit::deleteBcmUnitImpl() {
  if (attached_.load(std::memory_order_acquire)) {
    steady_clock::time_point begin = steady_clock::now();
    XLOG(DBG2) << " [Exit] Initiating BRCM ASIC shutdown";

    /* This is the counterpart of bcm_pktio_init() in the desctruction
     * path internal to SDK.  But Broadcom only made bcm_pktio_init
     * call in the SDK, not this one, so we have to make this call
     * here.  Broadcom is working on fixing it.
     */
    if (usePKTIO_) {
      int rv;
#ifdef INCLUDE_PKTIO
      rv = bcm_pktio_cleanup(unit_);
#else
      rv = BCM_E_CONFIG;
#endif
      bcmCheckError(rv, "Failed to cleanup PKTIO");
    }

    // Since this is a destructor, we can't use the virtual method:
    // platform_->getAsic()->isSupported(HwAsic::Feature::HSDK).
    // Otherwise it will complain "pure virtual method called"
    if (BcmAPI::isHwUsingHSDK()) {
      detachHSDK();
    } else {
      // Clean up SDK state, without touching the hardware
      int rv = _bcm_shutdown(unit_);
      bcmCheckError(
          rv, "failed to clean up BCM state during warm boot shutdown");

      if (!BcmAPI::isHwInSimMode()) {
        // Generally its good practice to invoke same APIs for both SIM/HW
        // but since SIM doesn't support warm-boot, invoking this function is
        // not needed Further, when this func is called for SIM, it hangs More
        // details CS00010405125
        rv = soc_shutdown(unit_);
        bcmCheckError(
            rv, "failed to clean up SDK state during warm boot shutdown");
      }
    }

    steady_clock::time_point bcmShutdownDone = steady_clock::now();
    XLOG(DBG2)
        << "[Exit] Bcm shut down time "
        << duration_cast<duration<float>>(bcmShutdownDone - begin).count();

    destroyHwUnit();

    attached_.store(false, std::memory_order_release);
  }

  // Unregister ourselves from BcmAPI.
  BcmAPI::unitDestroyed(this);
}

void BcmUnit::attachSDK6(bool warmBoot) {
  registerCallbackVector();

  int rv;
  if (warmBoot) {
    SOC_WARM_BOOT_START(unit_);
    rv = soc_init(unit_);
    bcmCheckError(rv, "failed to init unit ", unit_);
  } else {
    SOC_WARM_BOOT_DONE(unit_);
    rv = soc_reset_init(unit_);
    if (rv == BCM_E_TIMEOUT) {
      bcmLogError(rv, "failed to reset unit ", unit_);
      // exit with a special code so that we can detect this particular error
      // in the wrapper. I arbitrarily picked E_TIMEOUT's spot in the enum,
      // which is 9 counting starting from 0
      exit(9);
    }
    bcmCheckError(rv, "failed to reset unit ", unit_);
  }

  rv = soc_misc_init(unit_);
  bcmCheckError(rv, "failed to init misc for unit ", unit_);

  (static_cast<BcmSwitch*>(platform_->getHwSwitch()))
      ->runBcmScript(
          platform_->getDirectoryUtil()->getPersistentStateDir() + "/" +
          FLAGS_script_pre_bcm_attach);

  rv = soc_mmu_init(unit_);
  bcmCheckError(rv, "failed to init MMU for unit ", unit_);

  // bcmInit handles most of the device initialization.
  // bcmInit() will automatically complete warm boot--we don't need
  // to explicitly call SOC_WARM_BOOT_DONE().
  bcmInit();
}

void BcmUnit::attach(bool warmBoot) {
  if (attached_.load(std::memory_order_acquire)) {
    throw FbossError("unit ", unit_, " already attached");
  }

  if (platform_->getAsic()->isSupported(HwAsic::Feature::HSDK)) {
    attachHSDK(warmBoot);
  } else {
    attachSDK6(warmBoot);
  }
  // dump hw config after attach in case there was any modifications
  platform_->dumpHwConfig();

  auto asic = platform_->getAsic();

  if (asic->isSupported(HwAsic::Feature::PKTIO)) {
    if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK3) {
      usePKTIO_ = FLAGS_use_pktio;
    } else {
      usePKTIO_ = true;
    }
  }
  XLOG(DBG2) << "usePKTIO = " << usePKTIO_;

  attached_.store(true, std::memory_order_release);
} // namespace facebook::fboss

void BcmUnit::registerCallbackVector() {
  auto* dev = bde->get_dev(deviceIndex_);
  XLOG(DBG2) << "Initializing device " << deviceIndex_ << ": type "
             << dev->device << " rev " << (int)dev->rev;

  // Initialize the device driver function vector
  soc_cm_device_vectors_t vectors;
  memset(&vectors, 0, sizeof(vectors));
  bde->pci_bus_features(
      deviceIndex_,
      &vectors.big_endian_pio,
      &vectors.big_endian_packet,
      &vectors.big_endian_other);
  vectors.base_address = dev->base_address;
  vectors.config_var_get = bcmGetConfigVar;
  vectors.interrupt_connect = bdeInterruptConnect;
  vectors.interrupt_disconnect = bdeInterruptDisconnect;
  vectors.read = bdeRead;
  vectors.write = bdeWrite;
  vectors.pci_conf_read = bdePciConfRead;
  vectors.pci_conf_write = bdePciConfWrite;
  vectors.salloc = bdeSalloc;
  vectors.sfree = bdeSfree;
  vectors.sinval = bdeSinval;
  vectors.sflush = bdeSflush;
  vectors.l2p = bdeL2P_64;
  vectors.p2l = bdeP2L_64;
  vectors.iproc_read = bdeIprocRead;
  vectors.iproc_write = bdeIprocWrite;
  vectors.bus_type = bde->get_dev_type(deviceIndex_);
  vectors.spi_read = bdeSpiRead;
  vectors.spi_write = bdeSpiWrite;

  int rv = soc_cm_device_init(unit_, &vectors);
  bcmCheckError(rv, "failed to initialize unit ", unit_);
}

void BcmUnit::bcmInit() {
  steady_clock::time_point begin = steady_clock::now();
  XLOG(DBG2) << "Initializing BCM unit " << unit_;
  // Call bcm_attach()
  auto rv = bcm_attach(unit_, nullptr, nullptr, unit_);
  facebook::fboss::bcmCheckError(
      rv, "failed to init BCM driver for unit ", unit_);
  XLOG(DBG2)
      << "[Init] BRCM attach time "
      << duration_cast<duration<float>>(steady_clock::now() - begin).count();
}

void BcmUnit::rawRegisterWrite(uint16_t phyID, uint8_t reg, uint16_t data) {
  int rv = soc_miim_write(unit_, phyID, reg, data);
  facebook::fboss::bcmCheckError(
      rv, "Failed to write register ", reg, " on phy ", phyID);
}
} // namespace facebook::fboss
