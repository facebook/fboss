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

#include <folly/MacAddress.h>
#include <memory>

namespace facebook { namespace fboss {

class HwSwitch;
class SwSwitch;
class ThriftHandler;
struct ProductInfo;

/*
 * Platform represents a specific switch/router platform.
 *
 * Note that Platform is somewhat similar to the HwSwitch class--both contain
 * hardware-specific details.  However, HwSwitch contains data only about the
 * switching logic, while Platform contains all platform specific information,
 * beyond just the switching logic.  The Platform class itself isn't involved
 * in switching, it simply knows what type of HwSwitch to instantiate for this
 * platform.
 *
 * In general, multiple platforms may share the same HwSwitch implementation.
 * For instance, the BcmSwitch class is a HwSwitch implementation for systems
 * based on a single Broadcom ASIC.  Any platform with a single Broadcom ASIC
 * may use BcmSwitch, but they will likely each have their own Platform
 * implementation.
 */
class Platform {
 public:
  Platform() {}
  virtual ~Platform() {}

  /*
   * Get the HwSwitch for this platform.
   *
   * The HwSwitch object returned should be owned by the Platform, and must
   * remain valid for the lifetime of the Platform object.
   */
  virtual HwSwitch* getHwSwitch() const = 0;

  /*
   * onHwInitialized() will be called once the HwSwitch object has been
   * initialized.  Platform-specific initialization that requires access to the
   * HwSwitch can be performed here.
   */
  virtual void onHwInitialized(SwSwitch* sw) = 0;

  /*
   * Create the ThriftHandler.
   *
   * This will be invoked by fbossMain() during the initialization process.
   */
  virtual std::unique_ptr<ThriftHandler> createHandler(SwSwitch* sw) = 0;

  /*
   * Get the local MAC address for the switch.
   *
   * This method must be thread safe.  It may be called simultaneously from
   * various different threads.
   */
  virtual folly::MacAddress getLocalMac() const = 0;

  /*
   * Get the path to a directory where persistent state can be stored.
   *
   * Files written to this directory should be preserved across system reboots.
   */
  virtual std::string getPersistentStateDir() const = 0;

  /*
   * Get the product information
   */
  virtual void getProductInfo(ProductInfo& info) = 0;

  /*
   * Get the path to a directory where volatile state can be stored.
   *
   * Files written to this directory should be preserved across controller
   * restarts, but must be removed across system restarts.
   *
   * For instance, these files could be stored in a ramdisk.  Alternatively,
   * these could be stored in persistent storage with an init script that
   * empties the directory on reboot.
   */
  virtual std::string getVolatileStateDir() const = 0;

  /*
   * Get filename where switch state JSON maybe stored
   */
  std::string getWarmBootSwitchStateFile() const;
  /*
   * Get the directory where we will dump info when there is a crash.
   *
   * The directory is in persistent storage.
   */
  std::string getCrashInfoDir() const {
    return getPersistentStateDir() + "/crash";
  }

  /*
   * Get the directory where warm boot state is stored.
   */
  std::string getWarmBootDir() const {
    return getVolatileStateDir() + "/warm_boot";
  }
  /*
   * Get filename for where we dump hw state on crash
   */
  std::string getCrashHwStateFile() const;
  /*
   * Get filename for where we dump switch state on crash
   */
  std::string getCrashSwitchStateFile() const;

 private:
  // Forbidden copy constructor and assignment operator
  Platform(Platform const &) = delete;
  Platform& operator=(Platform const &) = delete;
};

}} // facebook::fboss
