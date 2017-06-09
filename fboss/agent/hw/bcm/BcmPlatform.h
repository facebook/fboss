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

#include "fboss/agent/Platform.h"
#include <map>
#include <string>

extern "C" {
#include <opennsl/types.h>
}

namespace facebook { namespace fboss {

class BcmPlatformPort;

/*
 * BcmPlatform specifies additional APIs that must be provided by platforms
 * based on Broadcom chips.
 */
class BcmPlatform : public Platform {
 public:
  typedef std::map<opennsl_port_t, BcmPlatformPort*> InitPortMap;

  BcmPlatform() {}

  /*
   * onUnitAttach() will be called by the BcmSwitch code immediately after
   * attaching to the switch unit.
   */
  virtual void onUnitAttach(int unit) = 0;

  /*
   * initPorts() will be called during port initialization.
   *
   * The BcmPlatform should return a map of BCM port ID to BcmPlatformPort
   * objects.  The BcmPlatform object will retain ownership of all the
   * BcmPlatformPort objects, and must ensure that they remain valid for as
   * long as the BcmSwitch exists.
   */
  virtual InitPortMap initPorts() = 0;

  /*
   * Get filename for where we dump the HW config that
   * the switch was initialized with.
   */
  virtual std::string getHwConfigDumpFile() const;

  /*
   * Based on the chip we may or may not be able to
   * use the host table for host routes (/128 or /32).
   */
  virtual bool canUseHostTableForHostRoutes() const = 0;
  TransceiverIdxThrift getPortMapping(PortID portId) const override = 0;

  /*
   * Get total device buffer in bytes
   */
  virtual uint32_t getMMUBufferBytes() const = 0;
  /*
   * MMU buffer is split into cells, each of which is
   * X bytes. Cells then serve as units of for allocation
   * and accounting of MMU resources.
   */
  virtual uint32_t getMMUCellBytes() const = 0;

  virtual bool isBufferStatsCollectionSupported() const = 0;

 private:
  // Forbidden copy constructor and assignment operator
  BcmPlatform(BcmPlatform const &) = delete;
  BcmPlatform& operator=(BcmPlatform const &) = delete;
};

}} // facebook::fboss
