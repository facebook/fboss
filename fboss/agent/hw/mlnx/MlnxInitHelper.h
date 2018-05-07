/* 
 * Copyright (c) Mellanox Technologies. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

#include "fboss/agent/hw/mlnx/MlnxConfig.h"

extern "C" {
#include <sx/sdk/sx_api.h>
#include <sx/sdk/sx_swid.h>
#include <sx/sxd/kernel_user.h>
}

#include <folly/Subprocess.h>

#include <sys/types.h>

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <atomic>

namespace facebook { namespace fboss {

class MlnxPlatform;
class MlnxSwitch;
class MlnxDevice;

/*
 * This class is a singleton
 * and is responsible for initializing the SDK
 */
class MlnxInitHelper {
 public:

  /**
   * Returns the MlnxInitHelper instance
   *
   * @return MlnxInitHelper instance
   */
  static MlnxInitHelper* get();

  /**
   * Loads configuration from xml file
   *
   * @param pathToConfigFile Path to xml file
   * @param localMac If not empty string overrides
   * @param device mac address provided by configuration
   */
  void loadConfig(const std::string& pathToConfigFile,
    const std::string& localMac);

  /**
   * Perform the SDK initialization
   */
  void init();

  /**
   * Deinitialize the SDK
   */
  void deinit();

  /**
   * Returns MlnxSwitch child instance depending on the ASIC type
   *
   * @ret MlnxSwitch instance
   */
  std::unique_ptr<MlnxSwitch> getMlnxSwitchInstance(MlnxPlatform* pltf);

  /**
   * Returns MlnxDevice object.
   * Note: as we support only one device in the system, this method
   * will throw when more than one is available.
   * MlnxDevice::init() method should be called to init device in sdk
   *
   * @param hw MlnxSwitch
   */
  std::unique_ptr<MlnxDevice> getSingleDevice(MlnxSwitch* hw);

  /**
   * Returns the SDK handle
   *
   * @return SDK handle
   */
  sx_api_handle_t getHandle() const;

  /**
   * Returns default switch partition id
   *
   * @return swid
   */
  sx_swid_t getDefaultSwid() const;

  /**
   * Returns chip type
   *
   * @ret chip type
   */
  sx_chip_types_t getChipType() const;

  /**
   * Returns chip resource limits struct
   *
   * @ret resource limits struct
   */
  const rm_resources_t& getRmLimits() const;

  /**
   * Returns mellanox configuration object
   *
   * @return MlnxConfig object
   */
  MlnxConfig& getConfig();

  /**
   * Method to pass to gflags validator.
   * Used to validate verbosity level of sdk module
   * (Checks if verbosity level is in set 'error', 'info', etc.)
   *
   * @param flagname name of the flag
   * @param verbosity value of the flag
   */
  static bool validateVerbosityLevel(const char* flagname,
      const std::string& verbosity);

 private:
  /**
   * ctor, Starts the sxdkernel service
   * NOTE: public deinit() method should be called
   *       explicitly to stop sxdkernel
   */
  MlnxInitHelper();

  // Forbidden copy constructor and assignment operator
  MlnxInitHelper(const MlnxInitHelper&) = delete;
  MlnxInitHelper& operator=(const MlnxInitHelper&) = delete;

  /**
   * Starts sxdkernel service
   */
  void restartSxdKernel();

  /**
   * Stops the sxdkernel service
   */
  void stopSxdKernel();

  /**
   * Initialize the SDK
   */
  void initSdk();

  /**
   * Starts the SDK process in background
   *
   */
  void startSdkProcess();

  /**
   * Interrupts sdk and acl_rm processes
   */
  void stopSdk();

  /**
   * wait until SDK becomes ready
   */
  void waitForSdk();

  /**
   * This function adds switch id's to the SDK
   */
  void enableSwids();

  /**
   * perform initialization flow
   */
  void initImpl();

  /**
   * Inits dpt shared memory and inits sxd access registers api
   */
  void initDptAndRegisterAccess();

  /**
   * Deinits dpt shared memory and deinits sxd access registers api
   */
  void deinitDptAndRegisterAccess();

  /**
   * Sets PCI, I2C, configuration path for EMAD and resets ASIC for single device,
   * which is directly attached to CPU through PCI bus
   *
   * @param devId Device id
   * @param deviceName Device name returned by sxd_get_dev_list
   */
  void configureSingleDevice(sxd_dev_id_t devId,
      std::array<char, MAX_NAME_LEN>& deviceName);

  /**
   * This function reads the ASIC type
   */
  void initChipType();

  /**
   * Sets verbosity levels of modules in SDK
   * according to command line arguments
   * NOTE: First sets the system verbosity level
   *       and then per each SDK module
   */
  void setVerbosityLevels();

  /**
   * Converts verbosity level string (error, info..) to sx_verbosity_level_t
   *
   * @param verbosityStr
   * @ret sx_verbosity_level_t enum value
   */
  sx_verbosity_level_t strToLogVerbosity(const std::string& verbosityStr);

  // private fields

  static const std::map<std::string, sx_verbosity_level_t> strToLogVerbosityMap;

  MlnxConfig mlnxConfig_;
  std::atomic<bool> configLoaded_ {false};

  // sx_sdk and sx_acl_rm processes
  folly::Subprocess sdkProcess_{};
  folly::Subprocess aclRmProcess_{};

  // only one switch partition
  const sx_swid_t kDefaultSwid_ {0};

  // SDK communication handle_
  sx_api_handle_t handle_ {SX_API_INVALID_HANDLE};
  // chip type found in the system
  sx_chip_types_t chipType_ {SX_CHIP_TYPE_UNKNOWN};
  // chip resource capabilities structure
  rm_resources_t resourceLimits_;
};

}}
