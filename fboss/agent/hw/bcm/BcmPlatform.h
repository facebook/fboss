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

#include <map>
#include <string>
#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"

extern "C" {
#include <bcm/types.h>
}

DECLARE_int32(qcm_ifp_gid);
DECLARE_int32(qcm_ifp_pri);
DECLARE_bool(load_qcm_fw);
DECLARE_string(qcm_fw_path);
DECLARE_bool(enable_qcm_ifp_statistics);
DECLARE_int32(init_gport_available_count);
DECLARE_string(hw_config_file);

namespace facebook::fboss {

class BcmPlatformPort;
class PortQueue;
class PortPgConfig;
class BufferPoolCfg;
class BcmPort;

/*
 * BcmPlatform specifies additional APIs that must be provided by platforms
 * based on Broadcom chips.
 */
class BcmPlatform : public Platform {
 public:
  typedef std::map<bcm_port_t, BcmPlatformPort*> BcmPlatformPortMap;

  using Platform::Platform;

  /*
   * onUnitCreate() will be called by the BcmSwitch code immediately after
   * creating up switch unit. This is distinct from actually attaching
   * to the unit/ASIC, which happens in a separate step.
   */
  virtual void onUnitCreate(int unit) = 0;

  /*
   * onUnitAttach() will be called by the BcmSwitch code immediately after
   * attaching to the switch unit.
   */
  virtual void onUnitAttach(int unit) = 0;

  /*
   * preWarmbootStateApplied() will be called by the BcmSwitch code immediately
   * before applying the warmboot state.
   */
  virtual void preWarmbootStateApplied() {}

  /*
   * The BcmPlatform should return a map of BCM port ID to BcmPlatformPort
   * objects.  The BcmPlatform object will retain ownership of all the
   * BcmPlatformPort objects, and must ensure that they remain valid for as
   * long as the BcmSwitch exists.
   */
  virtual BcmPlatformPortMap getPlatformPortMap() = 0;

  /*
   * Get filename for where we dump the HW config that
   * the switch was initialized with.
   */
  virtual std::string getHwConfigDumpFile() const;

  virtual bool supportsDynamicBcmConfig() const {
    return false;
  }

  /*
   * Based on the chip we may or may not be able to
   * use the host table for host routes (/128 or /32).
   */
  virtual bool canUseHostTableForHostRoutes() const = 0;
  TransceiverIdxThrift getPortMapping(PortID portId, cfg::PortSpeed speed)
      const override = 0;

  /*
   * Get total device buffer in bytes
   */
  virtual uint32_t getMMUBufferBytes() const = 0;
  /*
   * MMU buffer is split into cells, each of which is
   * X bytes. Cells then serve as units of for allocation
   * and accounting of MMU resources.
   */
  virtual uint32_t getMMUCellBytes() const override;

  virtual const PortQueue& getDefaultPortQueueSettings(
      cfg::StreamType streamType) const = 0;

  virtual const PortQueue& getDefaultControlPlaneQueueSettings(
      cfg::StreamType streamType) const = 0;

  virtual BcmWarmBootHelper* getWarmBootHelper() override {
    return warmBootHelper_.get();
  }

  virtual bool isBcmShellSupported() const;

  virtual bool isDisableHotSwapSupported() const;

  virtual bool useQueueGportForCos() const = 0;

  virtual int getPortItm(BcmPort* bcmPort) const;
  virtual const PortPgConfig& getDefaultPortPgSettings() const;
  virtual const BufferPoolCfg& getDefaultPortIngressPoolSettings() const;

  /*
    Ports have a concept of VCO frequency that is decided when port is first
    brought up, based on some of the port settings. Changing speed or other port
    settings sometimes will require a VCO frequency change, in which case we may
    not be able to use the usual way of reporgramming ports in BcmPorts.
    This function calculates required VCO frequency given required port settings
    which we can use to decide if we need some customized logic
  */
  virtual phy::VCOFrequency getVCOFrequency(
      phy::VCOFrequencyFactor& factor) const;

  /*
   * Dump map containing switch h/w config as a key, value pair
   * to a file. We dump in format that name, value pair format that
   * the SDK can read. Later this map is used to initialize the chip
   */
  void dumpHwConfig() const;

  virtual bool hasLinkScanCapability() const {
    return true;
  }

 protected:
  std::unique_ptr<BcmWarmBootHelper> warmBootHelper_;

 private:
  // Forbidden copy constructor and assignment operator
  BcmPlatform(BcmPlatform const&) = delete;
  BcmPlatform& operator=(BcmPlatform const&) = delete;
};

} // namespace facebook::fboss
