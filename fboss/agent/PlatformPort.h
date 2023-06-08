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

#include <folly/futures/Future.h>
#include <folly/io/async/EventBase.h>

#include "fboss/agent/gen-cpp2/platform_config_types.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/Transceiver.h"
#include "fboss/agent/types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

namespace facebook::fboss {

class Platform;

class PlatformPort {
 public:
  PlatformPort(PortID id, Platform* platform);
  PlatformPort(PlatformPort&&) = default;
  PlatformPort& operator=(PlatformPort&&) = default;
  virtual ~PlatformPort() {}

  /*
   * Get the PortID for this port
   */
  PortID getPortID() const {
    return id_;
  }
  /*
   * Get the Platform for this port
   */
  Platform* getPlatform() const {
    return platform_;
  }

  virtual std::optional<int> getAttachedCoreId() const;
  virtual std::optional<int> getCorePortIndex() const;

  const cfg::PlatformPortEntry& getPlatformPortEntry() const;
  cfg::PortType getPortType() const;

  std::optional<std::vector<phy::PinConfig>> getTransceiverPinConfigs(
      cfg::PortProfileID profileID) const;

  phy::PortPinConfig getPortXphyPinConfig(cfg::PortProfileID profileID) const;

  phy::PortPinConfig getPortPinConfigs(cfg::PortProfileID profileID) const;

  std::map<std::string, phy::DataPlanePhyChip> getPortDataplaneChips(
      cfg::PortProfileID profileID) const;

  cfg::PortProfileID getProfileIDBySpeed(cfg::PortSpeed speed) const;
  std::optional<cfg::PortProfileID> getProfileIDBySpeedIf(
      cfg::PortSpeed speed) const;

  /*
   * Helper functions for getting port profiles. Platform port generall should
   * have all information needed to create profile matchers, so we just need
   * profile id here
   * TODO(joseph5wu) Eventually we don't need to use platform port to fetch
   * TransceiverInfo, and use SwitchState TransceiverMap to calculate platform
   * config. Will remove these helper functions once we have the new port
   * programming logic ready in qsfp_service.
   */
  const phy::PortProfileConfig getPortProfileConfig(
      cfg::PortProfileID PortProfileID) const;

  const std::optional<phy::PortProfileConfig> getPortProfileConfigIf(
      cfg::PortProfileID PortProfileID) const;

  const phy::PortProfileConfig getPortProfileConfigFromCache(
      cfg::PortProfileID PortProfileID);

  /*
   * preDisable() will be called by the hardware code just before disabling
   * a port.
   *
   * The "temporary" parameter will be true if the port is configured to be
   * enabled, but is only being disabled temporarily to make some settings
   * changes.  (For instance, to change the port speed or interface type.)
   */
  virtual void preDisable(bool temporary) = 0;
  /*
   * postDisable() will be called by the hardware code just after disabling a
   * port.
   */
  virtual void postDisable(bool temporary) = 0;

  /*
   * preEnable() will be called by the hardware code just before enabling
   * a port.
   */
  virtual void preEnable() = 0;
  /*
   * postEnable() will be called by the hardware code just after enabling
   * a port.
   */
  virtual void postEnable() = 0;

  /*
   * isMediaPresent() must be implemented by the PlatformPort subclass,
   * and should return if media is present in the port.
   *
   * For instance, for a port connected to an SFP+ module, this should indicate
   * if a module is plugged in.  For an internal backplane port, this should
   * indicate if the remote fabric or line card is present.
   *
   * This is primarily used to indicate if link is expected to be up on the
   * port or not.  If no media is present, it is expected that the link will be
   * down.  If media is present but the link is down, the hardware code may try
   * periodically tuning the port parameters to see if it can achieve link.
   */
  virtual bool isMediaPresent() = 0;

  /*
   * Status indication functions.  These are mostly intended to allow
   * controlling LED indicators about the port.
   */

  /*
   * linkStatusChanged() will be called by the hardware code whenever
   * the link status changed.
   *
   * up indicates if link is present.
   * adminUp indicates if the port is enabled or disabled.
   */
  virtual void linkStatusChanged(bool up, bool adminUp) = 0;

  /*
   * Returns true if the port supports/expects to use a transceiver
   * and false if there is a direct electrical connection
   */
  virtual bool supportsTransceiver() const = 0;

  /*
   * Returns the transceiver's id if the port supports a transceiver,
   * otherwise, returns an empty std::optional
   */
  std::optional<TransceiverID> getTransceiverID() const {
    return transceiverID_;
  }

  /*
   * Returns the hardware logical port id if the port supports it,
   * otherwise, returns an empty std::optional
   */
  std::optional<uint32_t> getHwLogicalPortId() const {
    return hwLogicalPortId_;
  }

  void setHwLogicalPortId(uint32_t hwLogicalPortId) {
    hwLogicalPortId_ = hwLogicalPortId;
  }

  /*
   * statusIndication() will be called by the hardware code once a second.
   *
   * enabled  - If the port is enabled or not.
   * link     - If link is up on the port.
   * ingress  - If any packets have been received on the port since the last
   *            statusIndication() call.
   * egress   - If any packets have been sent on the port since the last
   *            statusIndication() call.
   * discards - If any packets have been discarded on the port since the last
   *            statusIndication() call.
   * errors   - If any packet errors have occurred on the port since the last
   *            statusIndication() call.
   */
  virtual void statusIndication(
      bool enabled,
      bool link,
      bool ingress,
      bool egress,
      bool discards,
      bool errors) = 0;
  /**
   * Do platform specific actions for the port before we are warm booting.
   */
  virtual void prepareForGracefulExit() = 0;

  /*
   * There are situations (e.g. backplane ports on some platforms)
   * where we don't need to enable FEC for ports even on 100G speeds.
   * Not enabling FEC on these means that we don't incur the (very)
   * minor latency (100ns) that enabling FEC entails.
   * So, provide derived class platform ports a hook to override
   * decision to enable FEC.
   *
   */
  virtual bool shouldDisableFEC() const = 0;

  /**
   * External conditions, outside of the port itself, may have significance
   * for the port.  Calling this function allows the port to take any debug
   * or diagnostic action needed, as appropriate.
   *
   * - For NONE, the port LEDs again show the internal port state (normal op)
   *
   * - For CABLING_ERROR, the platform-port sub-class may set the LEDs
   *   in some way to signal the issue to physically present operators.
   *
   * - For EXTERNAL_FORCE_ON/OFF, the LED control on a port is provided through
   *   a Thrift call to provide a mechanism for the automation to indicate the
   *   problematic port, cable or provide any other information to the operator.
   */
  virtual void externalState(PortLedExternalState) = 0;

  virtual folly::Future<TransceiverInfo> getFutureTransceiverInfo() const = 0;
  virtual std::shared_ptr<TransceiverSpec> getTransceiverSpec() const = 0;
  std::optional<TransceiverInfo> getTransceiverInfo(
      folly::EventBase* evb) const;

  std::optional<int32_t> getExternalPhyID();

  void clearCachedProfileConfig();

 protected:
  // Get Transceiver lanes from config
  // Return empty vector if the port doesn't support transceivers
  // Return non-empty vector to maintain the lanes of the transceivers
  std::vector<phy::PinID> getTransceiverLanes(
      std::optional<cfg::PortProfileID> profileID = std::nullopt) const;

  cfg::PlatformPortConfigOverrideFactor
  buildPlatformPortConfigOverrideFactorBySpec(
      const TransceiverSpec& transceiverSpec) const;

 private:
  // Forbidden copy constructor and assignment operator
  PlatformPort(PlatformPort const&) = delete;
  PlatformPort& operator=(PlatformPort const&) = delete;

  PortID id_{0};
  Platform* platform_{nullptr};
  // transceiver id should be fixed once the PlatformPort is created
  std::optional<TransceiverID> transceiverID_;
  std::optional<uint32_t> hwLogicalPortId_;

  // Cached port profile config when transceiver info is required
  folly::Synchronized<
      std::optional<std::pair<cfg::PortProfileID, phy::PortProfileConfig>>>
      cachedProfileConfig_;
};

std::ostream& operator<<(std::ostream&, PortLedExternalState);

/*
 * This is a base class for any platform port of a multi-pim system
 * For now, we use this class to store PimID and the transceiverIndexInPim
 */
class MultiPimPlatformPort {
 public:
  MultiPimPlatformPort(PortID id, const cfg::PlatformPortEntry& entry);
  PimID getPimID() const {
    return pimID_;
  }
  uint8_t getTransceiverIndexInPim() const {
    return transceiverIndexInPim_;
  }

 private:
  // Forbidden copy constructor and assignment operator
  MultiPimPlatformPort(MultiPimPlatformPort const&) = delete;
  MultiPimPlatformPort& operator=(MultiPimPlatformPort const&) = delete;

  PimID pimID_;
  uint8_t transceiverIndexInPim_;
};
} // namespace facebook::fboss
