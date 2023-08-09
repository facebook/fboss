// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/L2Entry.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

struct HwInitResult {
  std::shared_ptr<SwitchState> switchState{nullptr};
  std::unique_ptr<RoutingInformationBase> rib{nullptr};
  BootType bootType{BootType::UNINITIALIZED};
  float initializedTime{0.0};
  float bootTime{0.0};
};

// TODO - move this back to HwSwitchHandler after migration
// Done in stacked diffs
struct PlatformData {
  std::string volatileStateDir;
  std::string persistentStateDir;
  std::string crashSwitchStateFile;
  std::string crashThriftSwitchStateFile;
  std::string warmBootDir;
  std::string crashBadStateUpdateDir;
  std::string crashBadStateUpdateOldStateFile;
  std::string crashBadStateUpdateNewStateFile;
  std::string runningConfigDumpFile;
  bool supportsAddRemovePort;
};

class StateObserver;

class HwSwitchCallback {
 public:
  virtual ~HwSwitchCallback() {}

  /*
   * packetReceived() is invoked by the HwSwitch when a trapped packet is
   * received.
   */
  virtual void packetReceived(std::unique_ptr<RxPacket> pkt) noexcept = 0;

  /*
   * linkStateChanged() is invoked by the HwSwitch whenever the link
   * status changes on a port.
   */
  virtual void linkStateChanged(
      PortID port,
      bool up,
      std::optional<phy::LinkFaultStatus> iPhyFaultStatus = std::nullopt) = 0;

  /*
   * l2LearningUpdateReceived() is invoked by the HwSwitch when there is
   * changes l2 table.
   */
  virtual void l2LearningUpdateReceived(
      L2Entry l2Entry,
      L2EntryUpdateType l2EntryUpdateType) = 0;

  /*
   * Used to notify the SwSwitch of a fatal error so the implementation can
   * provide special behavior when a crash occurs.
   */
  virtual void exitFatal() const noexcept = 0;

  virtual void pfcWatchdogStateChanged(
      const PortID& port,
      const bool deadlock) = 0;

  virtual void registerStateObserver(
      StateObserver* observer,
      const std::string& name) = 0;

  virtual void unregisterStateObserver(StateObserver* observer) = 0;
};

using HwSwitchInitFn = std::function<HwInitResult(HwSwitchCallback*, bool)>;

} // namespace facebook::fboss
