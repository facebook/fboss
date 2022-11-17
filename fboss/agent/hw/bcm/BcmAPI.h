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

#include "fboss/agent/hw/bcm/types.h"
#include "fboss/agent/platforms/common/utils/BcmYamlConfig.h"

#include <map>
#include <memory>
#include <string>

#include <folly/Range.h>
#include <folly/experimental/StringKeyedUnorderedMap.h>
#include <yaml-cpp/yaml.h>

// defined in fboss/agent/hw/bcm/BcmUnit.cpp
DECLARE_bool(use_pktio);

namespace facebook::fboss {

class BcmUnit;
class BcmPlatform;

/*
 * BcmAPI is a singleton class that wraps the Broadcom SDK.
 *
 * It is primarily responsible for initializing the SDK and implementing
 * callbacks required by the SDK.
 *
 * This class only handles initialization of the SDK itself, and does not
 * initialize any Broadcom devices found in the system.  Use the BcmUnit class
 * for initializing individual devices.
 */
class BcmAPI {
 public:
  /*
   * Initialize the Broadcom SDK and create the BcmAPI singleton.
   *
   * This must be called before using any other Broadcom SDK functions.
   */
  static void init(const std::map<std::string, std::string>& config);

  /*
   * Initialize the Broadcom HSDK and create the BcmAPI singleton.
   *
   * This must be called before using any other Broadcom SDK functions.
   */
  static void initHSDK(const std::string& yamlConfig);

  /*
   * Shut the Broadcom SDK.
   *
   * This must be called after stop using any Broadcom SDK functions.
   * And it should only be called once we shut down BcmUnit
   */
  static void shutdown();

  /*
   * Get the number of Broadcom switching devices in this system.
   */
  static size_t getNumSwitches();

  /*
   * Get the maximum number of Broadcom switching devices in this system.
   */
  static size_t getMaxSwitches();

  /*
   * Create a BcmUnit.
   *
   * The unit will not have been initialized yet, and must still
   * be initialized with BcmUnit::attach().
   *
   * All devices should be initialized from the main thread, before
   * performing BCM SDK calls from other threads.  The Broadcom SDK does
   * not appear to perform locking around device ID allocation and
   * initialization.
   */
  static std::unique_ptr<BcmUnit> createUnit(
      int deviceIndex,
      BcmPlatform* platform);

  /*
   * Ensure that there is only a single BCM switch in the system,
   * and create a BcmUnit for it.
   *
   * The unit will not have been initialized yet, and must still
   * be initialized with BcmUnit::attach().
   */
  static std::unique_ptr<BcmUnit> createOnlyUnit(BcmPlatform* platform);

  /*
   * Initialize a BcmUnit.
   *
   * The unit will not have been initialized yet, and must still
   * be initialized with BcmUnit::attach().
   *
   * All devices should be initialized from the main thread, before
   * performing BCM SDK calls from other threads.  The Broadcom SDK does
   * not appear to perform locking around device ID allocation and
   * initialization.
   */
  static void initUnit(int unit, BcmPlatform* platform);

  /*
   * Indicate that a BcmUnit object is being destroyed.
   *
   * This should only be called by the BcmUnit destructor, and shouldn't
   * ever be called manually.
   */
  static void unitDestroyed(BcmUnit* unit);

  /*
   * Get a pointer to the BcmUnit for the specified unit number.
   *
   * Note that it is up to the caller to provide any required memory management
   * and locking for this BcmUnit object, and ensure that the BcmUnit is not
   * deleted by another thread while the caller is using it.
   *
   * Throws an FbossError if there is no BcmUnit for the specified unit number.
   */
  static BcmUnit* getUnit(int unit);

  /*
   * Get the thread name defined for this thread by the Broadcom SDK.
   */
  static std::string getThreadName();

  /*
   * Since Broadcom might have different formats for its config, we should move
   * all bcm config related getter functions here.
   * For example, in SDK6, we use "l3_alpm_enable" to set alpm enabling mode,
   * but in HSDK, we use "l3_alpm_template". And the values of both cases can
   * mean different modes.
   */
  static BcmMmuState getMmuState();

  static bool is128ByteIpv6Enabled();

  static bool isAlpmEnabled();

  static bool isPriorityKeyUsedInRxCosqMapping();

  static uint64_t getConfigStableSize();

  /*
   * Get a configuration property.
   * NOTE: This function should be only called for SDK6 implementation
   *
   * Returns the configuration value, as specified in the map supplied
   * to BcmAPI::init().
   *
   * The returned StringPiece will point to null data if no value
   * is set for the specified property.
   */
  static const char* FOLLY_NULLABLE getConfigValue(folly::StringPiece name);

  /*
   * SDK6 bcm config map.
   */
  typedef folly::StringKeyedUnorderedMap<std::string> HwConfigMap;
  static HwConfigMap& getHwConfig();

  static bool isHwInSimMode();

  static BcmYamlConfig& getHwYamlConfig();

  static bool isHwUsingHSDK();

 private:
  // Forbidden copy constructor and assignment operator
  BcmAPI(BcmAPI const&) = delete;
  BcmAPI& operator=(BcmAPI const&) = delete;

  /*
   * Initialize the BcmConfig to hold the config values passed in.
   * We use these values to keep an idea of the bcm configuration
   * values we are currently using and provide these to the sdk.
   */
  static void initConfig(const std::map<std::string, std::string>& config);

  static void initYamlConfig(const std::string& yamlConfig);

  static void updateUsingOverrideMap(HwConfigMap& hwConfig);

  static void initImpl();

  static void bdeCreate();
  static void bdeCreateSim();

  static void initHSDKImpl(const std::string& yamlConfig);

  /*
   * A flag indicates whether we finish bcm initialization.
   */
  static std::atomic<bool> bcmInitialized_;
  /*
   * A flag indicates whether we use ngbde
   */
  static bool isNgbde_;
  /*
   * A flag indicates whether we use HSDK
   */
  static bool isHSDK_;
};

} // namespace facebook::fboss
