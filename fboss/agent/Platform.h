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

#include <folly/Conv.h>
#include <folly/MacAddress.h>
#include <folly/io/async/EventBase.h>
#include <memory>
#include "fboss/agent/PlatformPort.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/types.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

namespace facebook::fboss {

class AgentConfig;
class HwSwitch;
class SwSwitch;
class ThriftHandler;
struct ProductInfo;
class HwAsic;
class PlatformProductInfo;
enum class PlatformMode : char;

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
  explicit Platform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<PlatformMapping> platformMapping);
  virtual ~Platform();

  /*
   * Initialize this platform. We use a two-phase initialization
   * scheme since we often need fine grained control of when to
   * initialized the platform.
   *
   * The platform should also store an owning reference of the config
   * passed in. Passing it in through init makes it possible to
   * control platform initialization using the same config mechanism
   * as other parts of the agent.
   */
  void init(std::unique_ptr<AgentConfig> config, uint32_t hwFeaturesDesired);

  /*
   * Allows the platorm to run any necessary cleanup steps like
   * stopping threads.
   */
  virtual void stop() = 0;

  /*
   * Two ways to get the configuration of the switch. config() will
   * pull current running config, reload( ) will also reload the
   * latest config from the default source for this platform before
   * returning.
   */
  const AgentConfig* config();
  const AgentConfig* reloadConfig();
  void setConfig(std::unique_ptr<AgentConfig> config);

  const std::map<int32_t, cfg::PlatformPortEntry>& getPlatformPorts() const;

  /*
   * Get supported port speed profile config based on profile id and the
   * transceiver specification compliance code.
   * Return std::nullopt if the platform doesn't support such speed profile.
   */
  const std::optional<phy::PortProfileConfig> getPortProfileConfig(
      cfg::PortProfileID PortProfileID,
      std::optional<ExtendedSpecComplianceCode> transceiverSpecComplianceCode =
          std::nullopt) const;

  /*
   * Get list of supported profiles from platform config. Returns empty map if
   * no platform config is found.
   */
  const std::map<cfg::PortProfileID, phy::PortProfileConfig>&
  getSupportedProfiles();

  /*
   * Get supported data plane phy chip based on chip name.
   * Return std::nullopt if the platform doesn't support such phy chip.
   */
  const std::optional<phy::DataPlanePhyChip> getDataPlanePhyChip(
      std::string chipName) const;
  const std::map<std::string, phy::DataPlanePhyChip>& getDataPlanePhyChips()
      const;

  /*
   * Get port max speed based on the platform rather than hw switch.
   */
  cfg::PortSpeed getPortMaxSpeed(PortID portID) const;

  /*
   * Get the HwSwitch for this platform.
   *
   * The HwSwitch object returned should be owned by the Platform, and must
   * remain valid for the lifetime of the Platform object.
   */
  virtual HwSwitch* getHwSwitch() const = 0;

  /*
   * Get the product information
   */
  void getProductInfo(ProductInfo& info);

  /*
   * Get the product mode
   */
  PlatformMode getMode() const;

  /*
   * onHwInitialized() will be called once the HwSwitch object has been
   * initialized.  Platform-specific initialization that requires access to the
   * HwSwitch can be performed here.
   */
  virtual void onHwInitialized(SwSwitch* sw) = 0;

  /*
   * onInitialConfigApplied() will be called after the initial
   * configuration has been applied.  Platform-specific initialization
   * that needs to happen after this can be performed here.
   */
  virtual void onInitialConfigApplied(SwSwitch* sw) = 0;

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
  virtual folly::MacAddress getLocalMac() const;

  /*
   * Get the path to a directory where persistent state can be stored.
   *
   * Files written to this directory should be preserved across system reboots.
   */
  virtual std::string getPersistentStateDir() const = 0;

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
   * Get the directory where we will dump info when there is a crash.
   *
   * The directory is in persistent storage.
   */
  std::string getCrashInfoDir() const {
    return getPersistentStateDir() + "/crash";
  }

  /*
   * Directory where we store info about state updates that led to a crash
   */
  std::string getCrashBadStateUpdateDir() const {
    return getCrashInfoDir() + "/bad_update";
  }

  std::string getCrashBadStateUpdateOldStateFile() const {
    return getCrashBadStateUpdateDir() + "/old_state";
  }

  std::string getCrashBadStateUpdateNewStateFile() const {
    return getCrashBadStateUpdateDir() + "/new_state";
  }

  /*
   * Get location we dump the running config of the switch
   */
  std::string getRunningConfigDumpFile() const {
    return getPersistentStateDir() + "/running-agent.conf";
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
  /*
   * For a specific logical port, return the transceiver and channel
   * it represents if available
   */
  virtual TransceiverIdxThrift getPortMapping(PortID port, cfg::PortSpeed speed)
      const = 0;

  virtual PlatformPort* getPlatformPort(PortID port) const = 0;

  virtual HwAsic* getAsic() const = 0;

  /*
   * initPorts() will be called during port initialization.
   */
  virtual void initPorts() = 0;

  virtual bool supportsAddRemovePort() const {
    return false;
  }

  phy::FecMode getPhyFecMode(cfg::PortProfileID profileID) const;

  const PlatformMapping* getPlatformMapping() const {
    return platformMapping_.get();
  }

  void setPort2OverrideTransceiverInfo(
      const std::map<PortID, TransceiverInfo>& port2TransceiverInfo);
  std::optional<std::map<PortID, TransceiverInfo>>
  getPort2OverrideTransceiverInfo() const;

  std::optional<TransceiverInfo> getOverrideTransceiverInfo(PortID port) const;
  int getLaneCount(cfg::PortProfileID profile) const;

 private:
  /*
   * Subclasses can override this to do custom initialization. This is
   * called from init() and will be invoked before trying to
   * initialize SwSwitch or other objects. Usually this is where
   * vendor-specific APIs are instantiated and where a platform
   * creates the HwSwitch instance it must serve back to SwSwitch.
   */
  virtual void initImpl(uint32_t hwFeaturesDesired) = 0;

  std::unique_ptr<AgentConfig> config_;

  // Forbidden copy constructor and assignment operator
  Platform(Platform const&) = delete;
  Platform& operator=(Platform const&) = delete;

  const std::unique_ptr<PlatformProductInfo> productInfo_;
  const std::unique_ptr<PlatformMapping> platformMapping_;
  // the map to transceiver info for ports, this is to be used only for HwTests
  // under test environment, qsfp may be unavailable and this map is to mock
  // possible transceiver info data qsfp may returns
  std::optional<std::map<PortID, TransceiverInfo>>
      port2OverrideTransceiverInfo_;
};

} // namespace facebook::fboss
