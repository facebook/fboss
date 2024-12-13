/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/SwSwitch.h"
#include <optional>

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/AgentDirectoryUtil.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/AlpmUtils.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/ArpHandler.h"
#include "fboss/agent/Constants.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/FbossHwUpdateError.h"
#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/LinkConnectivityProcessor.h"
#include "fboss/agent/Utils.h"

#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/IPv4Handler.h"
#include "fboss/agent/IPv6Handler.h"
#include "fboss/agent/L2Entry.h"
#include "fboss/agent/LacpTypes.h"
#include "fboss/agent/LinkAggregationManager.h"
#include "fboss/agent/LldpManager.h"
#include "fboss/agent/LookupClassRouteUpdater.h"
#include "fboss/agent/LookupClassUpdater.h"
#include "fboss/agent/ResourceAccountant.h"
#include "fboss/agent/SwitchInfoUtils.h"
#include "fboss/agent/hw/HwSwitchWarmBootHelper.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/lib/phy/gen-cpp2/prbs_types.h"
#if FOLLY_HAS_COROUTINES
#include "fboss/agent/MKAServiceManager.h"
#endif
#include "fboss/agent/AclNexthopHandler.h"
#include "fboss/agent/BuildInfoWrapper.h"
#include "fboss/agent/DsfSubscriber.h"
#include "fboss/agent/FsdbSyncer.h"
#include "fboss/agent/HwSwitchThriftClientTable.h"
#include "fboss/agent/MPLSHandler.h"
#include "fboss/agent/MacTableManager.h"
#include "fboss/agent/MirrorManager.h"
#include "fboss/agent/MultiHwSwitchHandler.h"
#include "fboss/agent/MultiSwitchFb303Stats.h"
#include "fboss/agent/MultiSwitchPacketStreamMap.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/PacketLogger.h"
#include "fboss/agent/PacketObserver.h"
#include "fboss/agent/PhySnapshotManager.h"
#include "fboss/agent/PortStats.h"
#include "fboss/agent/PortUpdateHandler.h"
#include "fboss/agent/RemoteNeighborUpdater.h"
#include "fboss/agent/ResolvedNexthopMonitor.h"
#include "fboss/agent/ResolvedNexthopProbeScheduler.h"
#include "fboss/agent/RestartTimeTracker.h"
#include "fboss/agent/RouteUpdateLogger.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/StaticL2ForNeighborObserver.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/SwSwitchWarmBootHelper.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/SwitchStatsObserver.h"
#include "fboss/agent/TeFlowNexthopHandler.h"
#include "fboss/agent/TunManager.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/capture/PcapPkt.h"
#include "fboss/agent/capture/PktCaptureManager.h"
#include "fboss/agent/gen-cpp2/switch_config_types_custom_protocol.h"
#include "fboss/agent/hw/HwSwitchFb303Stats.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/MPLSHdr.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/platforms/common/PlatformMappingUtils.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/StateUpdateHelpers.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"
#include "fboss/util/Logging.h"

#include "fboss/lib/CommonFileUtils.h"

#include <fb303/ServiceData.h>
#include <folly/Demangle.h>
#include <folly/FileUtil.h>
#include <folly/GLog.h>
#include <folly/MacAddress.h>
#include <folly/MapUtil.h>
#include <folly/SocketAddress.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <folly/system/ThreadName.h>
#include <glog/logging.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include <chrono>
#include <condition_variable>
#include <exception>
#include <tuple>

using folly::EventBase;
using folly::SocketAddress;
using folly::StringPiece;
using folly::io::Cursor;
using folly::io::RWPrivateCursor;
using std::exception;
using std::lock_guard;
using std::make_unique;
using std::map;
using std::mutex;
using std::set;
using std::shared_ptr;
using std::string;
using std::unique_ptr;

using facebook::fboss::AgentConfig;
using facebook::fboss::SwitchSettings;
using facebook::fboss::cfg::SdkVersion;
using facebook::fboss::cfg::SwitchConfig;
using facebook::fboss::cfg::SwitchDrainState;
using facebook::fboss::cfg::SwitchInfo;
using facebook::fboss::cfg::SwitchType;
using facebook::fboss::DeltaFunctions::forEachChanged;

using namespace std::chrono;
using namespace apache::thrift;
using namespace apache::thrift::transport;
using namespace apache::thrift::protocol;

DEFINE_int32(thread_heartbeat_ms, 1000, "Thread heartbeat interval (ms)");
DEFINE_int32(
    distribution_timeout_ms,
    1000,
    "Timeout for sending to distribution_service (ms)");

DEFINE_bool(
    log_all_fib_updates,
    false,
    "Flag to turn on logging of all updates to the FIB");

DEFINE_int32(
    minimum_ethernet_packet_length,
    64,
    "Expected minimum ethernet packet length");

DEFINE_int32(
    fsdbStatsStreamIntervalSeconds,
    5,
    "Interval at which stats subscriptions are served");

DECLARE_bool(intf_nbr_tables);

DEFINE_int32(
    hwagent_base_thrift_port,
    5931,
    "The first thrift server port reserved for HwAgent");

DEFINE_bool(
    dsf_publisher_GR,
    false,
    "Flag to turn on GR behavior for DSF publisher");

DEFINE_bool(rx_sw_priority, false, "Enable rx packet prioritization");

DEFINE_int32(rx_pkt_thread_timeout, 100, "Rx packet thread timeout (ms)");

DEFINE_int32(max_l2_entries, 1000, "Maximum L2 entries supported");

using namespace facebook::fboss;
namespace {

/**
 * Transforms the IPAddressV6 to MacAddress. RFC 2464
 * 33:33:xx:xx:xx:xx (lower 32 bits are copied from addr)
 */
folly::MacAddress getMulticastMacAddr(folly::IPAddressV6 addr) {
  return folly::MacAddress::createMulticast(addr);
}

/**
 * Transforms IPAddressV4 to MacAddress. RFC 1112
 * 01:00:5E:0x:xx:xx - 01:00:5E:7x:xx:xx (lower 23 bits are copied from addr)
 */
folly::MacAddress getMulticastMacAddr(folly::IPAddressV4 addr) {
  std::array<uint8_t, 6> bytes = {{0x01, 0x00, 0x5E, 0x00, 0x00, 0x00}};
  auto addrBytes = addr.toBinary();
  bytes[3] = addrBytes[1] & 0x7f; // Take only 7 bits
  bytes[4] = addrBytes[2];
  bytes[5] = addrBytes[3];
  return folly::MacAddress::fromBinary(
      folly::ByteRange(bytes.begin(), bytes.end()));
}

facebook::fboss::PortStatus fillInPortStatus(
    const facebook::fboss::Port& port,
    const facebook::fboss::SwSwitch* sw) {
  facebook::fboss::PortStatus status;
  *status.enabled() = port.isEnabled();
  *status.up() = port.isUp();
  *status.speedMbps() = static_cast<int>(port.getSpeed());
  *status.profileID() = apache::thrift::util::enumName(port.getProfileID());
  *status.drained() = port.isDrained();

  try {
    status.transceiverIdx() = sw->getTransceiverIdxThrift(port.getID());
  } catch (const facebook::fboss::FbossError&) {
    // No problem, we just don't set the other info
  }
  return status;
}

auto constexpr kHwUpdateFailures = "hw_update_failures";

std::string getDrainStateChangedStr(
    const std::shared_ptr<facebook::fboss::SwitchState>& oldState,
    const std::shared_ptr<facebook::fboss::SwitchState>& newState,
    const facebook::fboss::HwSwitchMatcher& matcher) {
  auto oldActualSwitchDrainState = oldState->getSwitchSettings()
                                       ->getNodeIf(matcher.matcherString())
                                       ->getActualSwitchDrainState();
  auto newActualSwitchDrainState = newState->getSwitchSettings()
                                       ->getNodeIf(matcher.matcherString())
                                       ->getActualSwitchDrainState();

  return oldActualSwitchDrainState != newActualSwitchDrainState
      ? folly::to<std::string>(
            "[",
            apache::thrift::util::enumNameSafe(oldActualSwitchDrainState),
            "->",
            apache::thrift::util::enumNameSafe(newActualSwitchDrainState),
            "]")
      : folly::to<std::string>(
            apache::thrift::util::enumNameSafe(oldActualSwitchDrainState),
            " (UNCHANGED)");
}

std::string getAsicSdkVersion(const std::optional<SdkVersion>& sdkVersion) {
  return sdkVersion.has_value() ? (sdkVersion.value().get_asicSdk() != nullptr
                                       ? *(sdkVersion.value().get_asicSdk())
                                       : std::string("Not found"))
                                : std::string("Not found");
}

// Create string about upper/lower port threshold for draining/undraining
std::string getDrainThresholdStr(
    SwitchDrainState newState,
    const SwitchSettings& switchSettings) {
  if (newState == SwitchDrainState::UNDRAINED) {
    auto minLinks = switchSettings.getMinLinksToRemainInVOQDomain();
    return "drains at < " +
        (minLinks.has_value() ? std::to_string(minLinks.value()) : "N/A") + ")";
  } else {
    auto minLinks = switchSettings.getMinLinksToJoinVOQDomain();
    return "undrains at >= " +
        (minLinks.has_value() ? std::to_string(minLinks.value()) : "N/A") + ")";
  }
}

void accumulateHwAsicErrorStats(
    facebook::fboss::HwAsicErrors& accumulated,
    const facebook::fboss::HwAsicErrors& toAdd) {
  *accumulated.parityErrors() += toAdd.parityErrors().value();
  *accumulated.correctedParityErrors() += toAdd.correctedParityErrors().value();
  *accumulated.uncorrectedParityErrors() +=
      toAdd.uncorrectedParityErrors().value();
  *accumulated.asicErrors() += toAdd.asicErrors().value();
}

void accumulateFb303GlobalStats(
    facebook::fboss::HwSwitchFb303GlobalStats& accumulated,
    const facebook::fboss::HwSwitchFb303GlobalStats& toAdd) {
  *accumulated.tx_pkt_allocated() += toAdd.tx_pkt_allocated().value();
  *accumulated.tx_pkt_freed() += toAdd.tx_pkt_freed().value();
  *accumulated.tx_pkt_sent() += toAdd.tx_pkt_sent().value();
  *accumulated.tx_pkt_sent_done() += toAdd.tx_pkt_sent_done().value();
  *accumulated.tx_errors() += toAdd.tx_errors().value();
  *accumulated.tx_pkt_allocation_errors() +=
      toAdd.tx_pkt_allocation_errors().value();
  *accumulated.parity_errors() += toAdd.parity_errors().value();
  *accumulated.parity_corr() += toAdd.parity_corr().value();
  *accumulated.parity_uncorr() += toAdd.parity_uncorr().value();
  *accumulated.asic_error() += toAdd.asic_error().value();
  *accumulated.global_drops() += toAdd.global_drops().value();
  *accumulated.global_reachability_drops() +=
      toAdd.global_reachability_drops().value();
  *accumulated.packet_integrity_drops() +=
      toAdd.packet_integrity_drops().value();
  *accumulated.dram_enqueued_bytes() += toAdd.dram_enqueued_bytes().value();
  *accumulated.dram_dequeued_bytes() += toAdd.dram_dequeued_bytes().value();
  if (toAdd.dram_blocked_time_ns().has_value()) {
    uint64_t dramBlockedTime = accumulated.dram_blocked_time_ns().value_or(0);
    dramBlockedTime += toAdd.dram_blocked_time_ns().value();
    accumulated.dram_blocked_time_ns() = dramBlockedTime;
  }
  if (toAdd.vsq_resource_exhaustion_drops().has_value()) {
    int64_t drops = accumulated.vsq_resource_exhaustion_drops().value_or(0);
    drops += toAdd.vsq_resource_exhaustion_drops().value();
    accumulated.vsq_resource_exhaustion_drops() = drops;
  }
  *accumulated.fabric_reachability_missing() +=
      toAdd.fabric_reachability_missing().value();
  *accumulated.fabric_reachability_mismatch() +=
      toAdd.fabric_reachability_mismatch().value();
  *accumulated.switch_reachability_change() +=
      toAdd.switch_reachability_change().value();
}

void accumulateGlobalCpuStats(
    facebook::fboss::CpuPortStats& accumulated,
    const facebook::fboss::CpuPortStats& toAdd) {
  for (const auto& [queue, value] : toAdd.queueInPackets_().value()) {
    (*accumulated.queueInPackets_())[queue] += value;
  }
  for (const auto& [queue, value] : toAdd.queueInPackets_().value()) {
    (*accumulated.queueDiscardPackets_())[queue] += value;
  }
  for (const auto& [queue, name] : toAdd.queueToName_().value()) {
    (*accumulated.queueToName_())[queue] = name;
  }
}

DEFINE_dynamic_quantile_stat(
    port_fec_tail,
    "port.{}.fecTail",
    facebook::fb303::ExportTypeConsts::kNone,
    std::array<double, 1>{{1.0}});

void updatePhyFb303Stats(
    const std::map<facebook::fboss::PortID, facebook::fboss::phy::PhyInfo>&
        phyInfoMap) {
  for (auto& [portID, phyInfo] : phyInfoMap) {
    auto& phyStats = *phyInfo.stats();
    auto& phyState = *phyInfo.state();
    if (phyState.get_name().empty()) {
      continue;
    }
    if (auto pcs = phyStats.line()->pcs()) {
      if (auto fec = pcs->rsFec()) {
        auto preFECBer = fec->get_preFECBer();
        // Pre-FEC BER should be >= 0 and <= 1
        if (preFECBer < 0 || preFECBer > 1) {
          XLOG(ERR) << "Invalid preFECBer value: " << preFECBer
                    << " for port: " << phyState.get_name();
          continue;
        }
        // For a BER of 2.2e-10, we will just log the exponent -10 to FB303
        // It should be safe to set the exponent to -32 when BER is 0. A BER of
        // lower than e-32 would mean 0 errors at 10^32 MBPS for 1 second. This
        // speed is infeasible at this point
        int64_t preFECBerForFb303 = -32;
        if (preFECBer != 0) {
          preFECBerForFb303 = std::floor(std::log10(preFECBer));
        }
        facebook::fb303::fbData->setCounter(
            "port." + phyState.get_name() + ".preFecBerLog", preFECBerForFb303);
        if (auto fecTail = fec->fecTail()) {
          STATS_port_fec_tail.addValue(*fecTail, phyState.get_name());
        }
      }
    }
  }
}

bool isPortDrained(
    const std::shared_ptr<SwitchState>& state,
    const Port* port,
    SwitchID portSwitchId) {
  HwSwitchMatcher matcher(std::unordered_set<SwitchID>({portSwitchId}));
  const auto& switchSettings = state->getSwitchSettings()->getSwitchSettings(
      HwSwitchMatcher(std::unordered_set<SwitchID>({portSwitchId})));
  return switchSettings->isSwitchDrained() || port->isDrained();
}
} // anonymous namespace

namespace std {
template <>
struct hash<std::set<facebook::fboss::PortID>> {
  size_t operator()(const std::set<facebook::fboss::PortID>& portIds) const;
};

size_t hash<std::set<facebook::fboss::PortID>>::operator()(
    const std::set<facebook::fboss::PortID>& portIds) const {
  size_t seed = 0;
  for (const auto& portId : portIds) {
    boost::hash_combine(seed, static_cast<int32_t>(portId));
  }
  return seed;
}
} // namespace std

namespace facebook::fboss {

SwSwitch::SwSwitch(
    HwSwitchHandlerInitFn hwSwitchHandlerInitFn,
    const AgentDirectoryUtil* agentDirUtil,
    bool supportsAddRemovePort,
    const AgentConfig* config,
    const std::shared_ptr<SwitchState>& initialState)
    : sdkVersion_(getSdkVersionFromConfig(config)),
      multiHwSwitchHandler_(new MultiHwSwitchHandler(
          getSwitchInfoFromConfig(config),
          std::move(hwSwitchHandlerInitFn),
          this,
          sdkVersion_)),
      agentDirUtil_(agentDirUtil),
      supportsAddRemovePort_(supportsAddRemovePort),
      platformProductInfo_(
          std::make_unique<PlatformProductInfo>(FLAGS_fruid_filepath)),
      pktObservers_(new PacketObservers()),
      l2LearnEventObservers_(new L2LearnEventObservers()),
      arp_(new ArpHandler(this)),
      ipv4_(new IPv4Handler(this)),
      ipv6_(new IPv6Handler(this)),
      nUpdater_(new NeighborUpdater(this)),
      pcapMgr_(new PktCaptureManager(
          agentDirUtil_->getPersistentStateDir(),
          pktObservers_.get())),
      mirrorManager_(new MirrorManager(this)),
      mplsHandler_(new MPLSHandler(this)),
      packetLogger_(new PacketLogger(this)),
      routeUpdateLogger_(new RouteUpdateLogger(this)),
      resolvedNexthopMonitor_(new ResolvedNexthopMonitor(this)),
      resolvedNexthopProbeScheduler_(new ResolvedNexthopProbeScheduler(this)),
      remoteNeighborUpdater_(new RemoteNeighborUpdater(this)),
      portUpdateHandler_(new PortUpdateHandler(this)),
      lookupClassUpdater_(new LookupClassUpdater(this)),
      lookupClassRouteUpdater_(new LookupClassRouteUpdater(this)),
      staticL2ForNeighborObserver_(new StaticL2ForNeighborObserver(this)),
      macTableManager_(new MacTableManager(this)),
      phySnapshotManager_(new PhySnapshotManager(kIphySnapshotIntervalSeconds)),
      aclNexthopHandler_(new AclNexthopHandler(this)),
      teFlowNextHopHandler_(new TeFlowNexthopHandler(this)),
      dsfSubscriber_(new DsfSubscriber(this)),
      switchInfoTable_(getSwitchInfoFromConfig(config)),
      hwAsicTable_(
          new HwAsicTable(getSwitchInfoFromConfig(config), sdkVersion_)),
      scopeResolver_(
          new SwitchIdScopeResolver(getSwitchInfoFromConfig(config))),
      switchStatsObserver_(new SwitchStatsObserver(this)),
      resourceAccountant_(new ResourceAccountant(hwAsicTable_.get())),
      packetStreamMap_(new MultiSwitchPacketStreamMap()),
      swSwitchWarmbootHelper_(
          new SwSwitchWarmBootHelper(agentDirUtil_, hwAsicTable_.get())),
      hwSwitchThriftClientTable_(new HwSwitchThriftClientTable(
          FLAGS_hwagent_base_thrift_port,
          getSwitchInfoFromConfig(config))) {
  // Create the platform-specific state directories if they
  // don't exist already.
  utilCreateDir(agentDirUtil_->getVolatileStateDir());
  utilCreateDir(agentDirUtil_->getPersistentStateDir());
  try {
    platformProductInfo_->initialize();
    platformMapping_ =
        utility::initPlatformMapping(platformProductInfo_->getType());
  } catch (const std::exception& ex) {
    // Expected when fruid file is not of a switch (eg: on devservers)
    XLOG(INFO) << "Couldn't initialize platform mapping " << ex.what();
  }
  if (getScopeResolver()->hasMultipleSwitches()) {
    multiSwitchFb303Stats_ =
        std::make_unique<MultiSwitchFb303Stats>(getHwAsicTable()->getHwAsics());
  }
  fsdbSyncer_.withWLock(
      [this](auto& syncer) { syncer = std::make_unique<FsdbSyncer>(this); });
  if (initialState) {
    initialState->publish();
    setStateInternal(initialState);
    CHECK(getAppliedState());
  }
}

SwSwitch::SwSwitch(
    HwSwitchHandlerInitFn hwSwitchHandlerInitFn,
    std::unique_ptr<PlatformMapping> platformMapping,
    const AgentDirectoryUtil* agentDirUtil,
    bool supportsAddRemovePort,
    const AgentConfig* config,
    const std::shared_ptr<SwitchState>& initialState)
    : SwSwitch(
          std::move(hwSwitchHandlerInitFn),
          agentDirUtil,
          supportsAddRemovePort,
          config,
          initialState) {
  platformMapping_ = std::move(platformMapping);
}

SwSwitch::~SwSwitch() {
  if (getSwitchRunState() < SwitchRunState::EXITING) {
    // If we didn't already stop (say via gracefulExit call), begin
    // exit
    stop(false /* gracefulStop */);
    restart_time::stop();
  }
}

bool SwSwitch::fsdbStatPublishReady() const {
  return fsdbSyncer_.withRLock(
      [](const auto& syncer) { return syncer->isReadyForStatPublishing(); });
}

bool SwSwitch::fsdbStatePublishReady() const {
  return fsdbSyncer_.withRLock(
      [](const auto& syncer) { return syncer->isReadyForStatePublishing(); });
}

void SwSwitch::stop(bool isGracefulStop, bool revertToMinAlpmState) {
  // Clean up connections to FSDB before stopping packet flow.
  runFsdbSyncFunction([isGracefulStop](auto& syncer) {
    syncer->stop(isGracefulStop);
    syncer.reset();
  });
  // Stop DSF subscriber to let us unsubscribe gracefully before stoppping
  // packet TX/RX functionality
  dsfSubscriber_->stop();

  setSwitchRunState(SwitchRunState::EXITING);

  XLOG(DBG2) << "Stopping SwSwitch...";

  // First tell the hw to stop sending us events by unregistering the callback
  // After this we should no longer receive packets or link state changed events
  // while we are destroying ourselves
  if (isRunModeMonolithic()) {
    getMonolithicHwSwitchHandler()->unregisterCallbacks();
  }

  // Stop tunMgr so we don't get any packets to process
  // in software that were sent to the switch ip or were
  // routed from kernel to the front panel tunnel interface.
  if (tunMgr_) {
    tunMgr_->stopProcessing();
  }

  resolvedNexthopMonitor_.reset();
  resolvedNexthopProbeScheduler_.reset();
  remoteNeighborUpdater_.reset();
  // Several member variables are performing operations in the background
  // thread.  Ask them to stop, before we shut down the background thread.
  //
  // It should be safe to destroy a StateObserver object without locking
  // if they are only ever accessed from the update thread. The destructor
  // of a class that extends AutoRegisterStateObserver will unregister
  // itself on the update thread so there should be no race. Unfortunately,
  // this is not the case for ipv6_ and tunMgr_, which may be accessed in a
  // packet handling callback as well while stopping the switch.
  //
  nUpdater_.reset();

  if (lldpManager_) {
    lldpManager_->stop();
  }

  if (lagManager_) {
    lagManager_.reset();
  }

  // Need to destroy IPv6Handler as it is a state observer,
  // but we must do it after we've stopped TunManager.
  // Otherwise, we might attempt to call sendL3Packet which
  // calls ipv6_->sendNeighborSolicitation which will then segfault
  ipv6_.reset();

  packetLogger_.reset();
  routeUpdateLogger_.reset();

  if (heartbeatWatchdog_) {
    heartbeatWatchdog_->stop();
    heartbeatWatchdog_.reset();
  }
  bgThreadHeartbeat_.reset();
  updThreadHeartbeat_.reset();
  packetTxThreadHeartbeat_.reset();
  lacpThreadHeartbeat_.reset();
  neighborCacheThreadHeartbeat_.reset();
  if (rib_) {
    rib_->stop();
  }

  // free dsfSubscriber_ only after thread heartbeats are freed.
  dsfSubscriberReconnectThreadHeartbeat_.reset();
  dsfSubscriberStreamThreadHeartbeat_.reset();
  dsfSubscriber_.reset();

  lookupClassUpdater_.reset();
  lookupClassRouteUpdater_.reset();
  macTableManager_.reset();
#if FOLLY_HAS_COROUTINES
  mkaServiceManager_.reset();
#endif
  phySnapshotManager_.reset();

  // stops the background and update threads.
  stopThreads();

  // reset explicitly since it uses observer. Make sure to reset after bg thread
  // is stopped, else we'll race with bg thread sending packets such as route
  // advertisements
  pcapMgr_.reset();
  pktObservers_.reset();
  l2LearnEventObservers_.reset();

  // reset tunnel manager only after pkt thread is stopped
  // as there could be state updates in progress which will
  // access entries in tunnel manager
  tunMgr_.reset();

  // ALPM requires default routes be deleted at last. Thus,
  // blow away all routes except the min required for ALPM,
  // so as to properly destroy hw switch. This is part of
  // exit logic, but updateStateBlocking() won't work in EXITING
  // state. Thus, directly calling underlying getHw_DEPRECATED()->stateChanged()
  if (revertToMinAlpmState) {
    XLOG(DBG3) << "setup min ALPM state";
    stateChanged(
        StateDelta(getState(), getMinAlpmRouteState(getState())), false);
  }
}

bool SwSwitch::isFullyInitialized() const {
  auto state = getSwitchRunState();
  return state >= SwitchRunState::INITIALIZED &&
      state != SwitchRunState::EXITING;
}

bool SwSwitch::isInitialized() const {
  return getSwitchRunState() >= SwitchRunState::INITIALIZED;
}

bool SwSwitch::isFullyConfigured() const {
  auto state = getSwitchRunState();
  return state >= SwitchRunState::CONFIGURED &&
      state != SwitchRunState::EXITING;
}

bool SwSwitch::isConfigured() const {
  return getSwitchRunState() >= SwitchRunState::CONFIGURED;
}

bool SwSwitch::isExiting() const {
  return getSwitchRunState() >= SwitchRunState::EXITING;
}

void SwSwitch::setSwitchRunState(SwitchRunState runState) {
  auto oldState = runState_.exchange(runState, std::memory_order_acq_rel);
  CHECK(oldState <= runState);
  onSwitchRunStateChange(runState);
  logSwitchRunStateChange(oldState, runState);
}

void SwSwitch::onSwitchRunStateChange(SwitchRunState newState) {
  if (newState == SwitchRunState::INITIALIZED) {
    restart_time::mark(RestartEvent::INITIALIZED);
  } else if (newState == SwitchRunState::CONFIGURED) {
    restart_time::mark(RestartEvent::CONFIGURED);
  }
  auto updateFn = [newState](const shared_ptr<SwitchState>& state) {
    auto newSwitchState = state->clone();
    for (const auto& [_, switchSettings] :
         std::as_const(*newSwitchState->getSwitchSettings())) {
      auto newSwitchSettings = switchSettings->modify(&newSwitchState);
      newSwitchSettings->setSwSwitchRunState(newState);
    }
    return newSwitchState;
  };
  updateState("update switch runstate", updateFn);
  // For multiswitch, run state is part of switch state
  if (isRunModeMonolithic()) {
    getMonolithicHwSwitchHandler()->switchRunStateChanged(newState);
  }
}

SwitchRunState SwSwitch::getSwitchRunState() const {
  return runState_.load(std::memory_order_acquire);
}

void SwSwitch::setFibSyncTimeForClient(ClientID clientId) {
  switch (clientId) {
    case ClientID::BGPD:
      restart_time::mark(RestartEvent::FIB_SYNCED_BGPD);
      break;
    case ClientID::OPENR:
      restart_time::mark(RestartEvent::FIB_SYNCED_OPENR);
      break;
    default:
      // no time tracking for remaining clients
      break;
  }
}

state::WarmbootState SwSwitch::gracefulExitState() const {
  state::WarmbootState thriftSwitchState;
  // For RIB we employ a optmization to serialize only unresolved routes
  // and recover others from FIB
  thriftSwitchState.routeTables() = rib_->warmBootState();
  *thriftSwitchState.swSwitchState() = getAppliedState()->toThrift();
  return thriftSwitchState;
}

void SwSwitch::gracefulExit() {
  if (isFullyInitialized()) {
    steady_clock::time_point begin = steady_clock::now();
    XLOG(DBG2) << "[Exit] Starting SwSwitch graceful exit";
    ipv6_->floodNeighborAdvertisements();
    arp_->floodGratuituousArp();
    steady_clock::time_point neighborFloodDone = steady_clock::now();
    XLOG(DBG2)
        << "[Exit] Neighbor flood time "
        << duration_cast<duration<float>>(neighborFloodDone - begin).count();
    // Stop handlers and threads before uninitializing h/w
    bool canWarmBoot{FLAGS_dsf_publisher_GR};
    stop(canWarmBoot);
    steady_clock::time_point stopThreadsAndHandlersDone = steady_clock::now();
    XLOG(DBG2) << "[Exit] Stop thread and handlers time "
               << duration_cast<duration<float>>(
                      stopThreadsAndHandlersDone - neighborFloodDone)
                      .count();

    auto thriftSwitchState = gracefulExitState();
    // write exit state
    steady_clock::time_point switchStateToFollyDone = steady_clock::now();
    XLOG(DBG2) << "[Exit] Switch state to folly dynamic "
               << duration_cast<duration<float>>(
                      switchStateToFollyDone - stopThreadsAndHandlersDone)
                      .count();
    // Cleanup if we ever initialized
    stopHwSwitchHandler();
    storeWarmBootState(thriftSwitchState);
    XLOG(DBG2)
        << "[Exit] SwSwitch Graceful Exit time "
        << duration_cast<duration<float>>(steady_clock::now() - begin).count();
  }
}

void SwSwitch::getProductInfo(ProductInfo& productInfo) const {
  CHECK(platformProductInfo_);
  platformProductInfo_->getInfo(productInfo);
}

PlatformType SwSwitch::getPlatformType() const {
  CHECK(platformProductInfo_);
  return platformProductInfo_->getType();
}

bool SwSwitch::getPlatformSupportsAddRemovePort() const {
  return supportsAddRemovePort_;
}

void SwSwitch::publishPhyInfoSnapshots(PortID portID) const {
  phySnapshotManager_->publishSnapshots(portID);
}

void SwSwitch::updateLldpStats() {
  if (!lldpManager_) {
    return;
  }

  // Use number of entries left after pruning expired entries
  stats()->LldpNeighborsSize(lldpManager_->getDB()->pruneExpiredNeighbors());
}

HwBufferPoolStats SwSwitch::getBufferPoolStatsFromSwitchWatermarkStats() {
  uint64_t deviceWatermarkBytes{0};
  auto lockedStats = hwSwitchStats_.wlock();
  for (auto& [switchIdx, hwSwitchStats] : *lockedStats) {
    deviceWatermarkBytes = std::max<uint64_t>(
        deviceWatermarkBytes,
        *hwSwitchStats.switchWatermarkStats()->deviceWatermarkBytes());
  }
  HwBufferPoolStats stats{};
  stats.deviceWatermarkBytes() = deviceWatermarkBytes;
  return stats;
}

AgentStats SwSwitch::fillFsdbStats() {
  AgentStats agentStats;
  {
    auto lockedStats = hwSwitchStats_.wlock();
    // fill stats using hwswitch exported data if available
    for (auto& [switchIdx, hwSwitchStats] : *lockedStats) {
      // accumulate error stats from all switches in global values
      accumulateHwAsicErrorStats(
          *agentStats.hwAsicErrors(), *hwSwitchStats.hwAsicErrors());

      for (auto&& statEntry : *hwSwitchStats.hwPortStats()) {
        agentStats.hwPortStats()->insert(statEntry);
      }
      for (auto&& statEntry : *hwSwitchStats.hwTrunkStats()) {
        agentStats.hwTrunkStats()->insert(statEntry);
      }
      agentStats.hwResourceStatsMap()->insert(
          {switchIdx, *hwSwitchStats.hwResourceStats()});
      agentStats.hwAsicErrorsMap()->insert(
          {switchIdx, *hwSwitchStats.hwAsicErrors()});
      agentStats.teFlowStatsMap()->insert(
          {switchIdx, *hwSwitchStats.teFlowStats()});
      agentStats.sysPortStatsMap()->insert(
          {switchIdx, *hwSwitchStats.sysPortStats()});
      agentStats.switchDropStatsMap()->insert(
          {switchIdx, *hwSwitchStats.switchDropStats()});
      for (auto& [_, phyInfo] : *hwSwitchStats.phyInfo()) {
        auto portName = phyInfo.state()->name().value();
        agentStats.phyStats()->insert({portName, phyInfo.get_stats()});
      }
      agentStats.flowletStatsMap()->insert(
          {switchIdx, *hwSwitchStats.flowletStats()});
      agentStats.cpuPortStatsMap()->insert(
          {switchIdx, *hwSwitchStats.cpuPortStats()});
      agentStats.switchWatermarkStatsMap()->insert(
          {switchIdx, *hwSwitchStats.switchWatermarkStats()});
      agentStats.fabricReachabilityStatsMap()->insert(
          {switchIdx, *hwSwitchStats.fabricReachabilityStats()});
    }
  }
  stats()->fillAgentStats(agentStats);
  getHwSwitchHandler()->fillHwAgentConnectionStatus(agentStats);
  // fill old fields using first switch values for backward compatibility
  agentStats.hwResourceStats() =
      agentStats.hwResourceStatsMap()->begin()->second;
  agentStats.teFlowStats() = agentStats.teFlowStatsMap()->begin()->second;
  agentStats.sysPortStats() = agentStats.sysPortStatsMap()->begin()->second;
  agentStats.flowletStats() = agentStats.flowletStatsMap()->begin()->second;
  // TODO: Remove this once switchWatermarkStats is rolled out to fleet!
  agentStats.bufferPoolStats() = getBufferPoolStatsFromSwitchWatermarkStats();
  return agentStats;
}

void SwSwitch::publishStatsToFsdb() {
  auto agentStats = fillFsdbStats();
  runFsdbSyncFunction([&agentStats](auto& syncer) {
    syncer->statsUpdated(std::move(agentStats));
  });
}

MonolithicHwSwitchHandler* SwSwitch::getMonolithicHwSwitchHandler() const {
  CHECK(!isRunModeMultiSwitch())
      << "Monolithic switch handler access should not be attempted in multi switch mode!";
  auto hwSwitchHandlers = getHwSwitchHandler()->getHwSwitchHandlers();
  return static_cast<MonolithicHwSwitchHandler*>(
      hwSwitchHandlers.begin()->second);
}

int16_t SwSwitch::getSwitchIndexForInterface(
    const std::string& interface) const {
  auto portId = getPlatformMapping()->getPortID(interface);
  auto switchId = getScopeResolver()->scope(portId).switchId();
  return getSwitchInfoTable().getSwitchIndexFromSwitchId(switchId);
}

void SwSwitch::updateStats() {
  SCOPE_EXIT {
    if (FLAGS_publish_stats_to_fsdb) {
      auto now = std::chrono::steady_clock::now();
      if (!publishedStatsToFsdbAt_ ||
          std::chrono::duration_cast<std::chrono::seconds>(
              now - *publishedStatsToFsdbAt_)
                  .count() > FLAGS_fsdbStatsStreamIntervalSeconds) {
        publishStatsToFsdb();
        publishedStatsToFsdbAt_ = now;
      }
    }
  };
  updateRouteStats();
  updatePortInfo();
  updateLldpStats();
  updateTeFlowStats();
  updateMultiSwitchGlobalFb303Stats();
  stats()->maxNumOfPhysicalHostsPerQueue(
      getLookupClassUpdater()->getMaxNumHostsPerQueue());

  if (!isRunModeMultiSwitch()) {
    multiswitch::HwSwitchStats hwStats;
    auto monoHwSwitchHandler = getMonolithicHwSwitchHandler();
    try {
      monoHwSwitchHandler->updateStats();
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Error running updateStats: " << folly::exceptionStr(ex);
    }
    try {
      monoHwSwitchHandler->updateAllPhyInfo();
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Error running updateAllPhyInfo: "
                << folly::exceptionStr(ex);
    }
    monoHwSwitchHandler->getHwStats(hwStats);
    hwStats.teFlowStats() = getTeFlowStats();
    updateHwSwitchStats(0 /*switchIndex*/, std::move(hwStats));
  }
  updateFlowletStats();

  std::map<PortID, phy::PhyInfo> phyInfo;
  {
    auto lockedStats = hwSwitchStats_.rlock();
    for (auto& [_, hwSwitchStats] : *lockedStats) {
      for (auto& [portID, phyInfoEntry] : *hwSwitchStats.phyInfo()) {
        phyInfo.insert({PortID(portID), phyInfoEntry});
      }
    }
    auto state = getState();
    for (const auto& portMap : std::as_const(*state->getPorts())) {
      for (const auto& [_, port] : std::as_const(*portMap.second)) {
        auto portSwitchId = scopeResolver_->scope(port).switchId();
        auto portSwitchIdx =
            switchInfoTable_.getSwitchIndexFromSwitchId(portSwitchId);
        auto sitr = lockedStats->find(portSwitchIdx);
        if (sitr == lockedStats->cend()) {
          continue;
        }
        auto pitr = sitr->second.hwPortStats()->find(port->getName());
        if (pitr == sitr->second.hwPortStats()->cend()) {
          continue;
        }
        std::optional<bool> portActive;
        if (port->getActiveState().has_value()) {
          portActive = *port->getActiveState() == Port::ActiveState::ACTIVE;
        }
        auto portStat = portStats(port->getID());
        const auto& hwPortStats = pitr->second;
        auto portDrained = isPortDrained(state, port.get(), portSwitchId);
        portStat->inErrors(*hwPortStats.inErrors_(), portDrained, portActive);
        portStat->fecUncorrectableErrors(
            *hwPortStats.fecUncorrectableErrors(), portDrained, portActive);
      }
    }
  }

  phySnapshotManager_->updatePhyInfos(phyInfo);
  updatePhyFb303Stats(phyInfo);
}

void SwSwitch::updateRouteStats() {
  // updateRouteStats() could be called before we are getting the first state
  auto state = getState();
  if (!state) {
    return;
  }
  auto [v4Count, v6Count] = getRouteCount(state);
  fb303::fbData->setCounter(SwitchStats::kCounterPrefix + "routes.v4", v4Count);
  fb303::fbData->setCounter(SwitchStats::kCounterPrefix + "routes.v6", v6Count);
}

void SwSwitch::updateTeFlowStats() {
  // updateTeFlowStats() could be called before we are getting the first state
  auto state = getState();
  if (!state) {
    return;
  }
  auto multiTeFlowTable = state->getTeFlowTable();
  fb303::fbData->setCounter(
      SwitchStats::kCounterPrefix + "teflows", multiTeFlowTable->numNodes());
  auto inactiveFlows = 0;
  for (const auto& [_, teFlowTable] : std::as_const(*multiTeFlowTable)) {
    for (const auto& [flowStr, flow] : std::as_const(*teFlowTable)) {
      if (!flow->getEnabled()) {
        inactiveFlows++;
      }
    }
  }
  fb303::fbData->setCounter(
      SwitchStats::kCounterPrefix + "teflows.inactive", inactiveFlows);
}

void SwSwitch::updatePortInfo() {
  auto state = getState();
  if (!state) {
    return;
  }
  for (const auto& portMap : std::as_const(*state->getPorts())) {
    for (const auto& port : std::as_const(*portMap.second)) {
      fb303::fbData->setCounter(
          "port." + port.second->getName() + ".speed",
          static_cast<int64_t>(port.second->getSpeed()) * 1'000'000ul);
      std::string idKey =
          folly::to<std::string>("port.", port.second->getID(), ".name");
      fb303::fbData->setExportedValue(idKey, port.second->getName());
    }
  }
}

void SwSwitch::linkConnectivityChanged(
    const std::map<PortID, multiswitch::FabricConnectivityDelta>&
        connectivityDelta) {
  auto updateFn = [=, this](const shared_ptr<SwitchState>& state) {
    return LinkConnectivityProcessor::process(
        *getScopeResolver(), *getHwAsicTable(), state, connectivityDelta);
  };
  updateState("update fabric connectivity info", updateFn);
}

void SwSwitch::updateMultiSwitchGlobalFb303Stats() {
  // Stats aggregation done only when multiple switches are present
  if (!getScopeResolver()->hasMultipleSwitches()) {
    return;
  }
  HwSwitchFb303GlobalStats globalStats;
  CpuPortStats globalCpuPortStats;
  {
    auto lockedStats = hwSwitchStats_.rlock();
    if (lockedStats->empty()) {
      return;
    }
    for (auto& [switchIdx, hwSwitchStats] : *lockedStats) {
      // accumulate error stats from all switches in global values
      accumulateFb303GlobalStats(
          globalStats, *hwSwitchStats.fb303GlobalStats());
      accumulateGlobalCpuStats(
          globalCpuPortStats, *hwSwitchStats.cpuPortStats());
    }
  }
  CHECK(multiSwitchFb303Stats_);
  multiSwitchFb303Stats_->updateStats(globalStats);
  multiSwitchFb303Stats_->updateStats(globalCpuPortStats);
}

bool SwSwitch::isRunModeMultiSwitch() const {
  return FLAGS_multi_switch ||
      (*agentConfig_.rlock())->getRunMode() == cfg::AgentRunMode::MULTI_SWITCH;
}

void SwSwitch::getAllHwSysPortStats(
    std::map<std::string, HwSysPortStats>& hwSysPortStats) const {
  auto hwswitchStatsMap = hwSwitchStats_.rlock();
  for (const auto& [switchIdx, hwSwitchStats] : *hwswitchStatsMap) {
    for (const auto& [portName, hwSysPortStatsEntry] :
         *hwSwitchStats.sysPortStats()) {
      hwSysPortStats.emplace(portName, hwSysPortStatsEntry);
    }
  }
}

void SwSwitch::getAllHwPortStats(
    std::map<std::string, HwPortStats>& hwPortStats) const {
  auto hwswitchStatsMap = hwSwitchStats_.rlock();
  for (const auto& [switchIdx, hwSwitchStats] : *hwswitchStatsMap) {
    for (const auto& [portName, hwPortStatsEntry] :
         *hwSwitchStats.hwPortStats()) {
      hwPortStats.emplace(portName, hwPortStatsEntry);
    }
  }
}

void SwSwitch::getAllCpuPortStats(
    std::map<int, CpuPortStats>& hwCpuPortStats) const {
  auto hwswitchStatsMap = hwSwitchStats_.rlock();
  for (const auto& [switchIdx, hwSwitchStats] : *hwswitchStatsMap) {
    hwCpuPortStats.emplace(switchIdx, *hwSwitchStats.cpuPortStats());
  }
}

void SwSwitch::updateFlowletStats() {
  uint64_t dlbErrorPackets = 0;
  {
    auto lockedStats = hwSwitchStats_.rlock();
    for (auto& [switchIdx, hwSwitchStats] : *lockedStats) {
      dlbErrorPackets +=
          hwSwitchStats.flowletStats()->l3EcmpDlbFailPackets().value();
    }
  }
  fb303::fbData->setCounter(
      SwitchStats::kCounterPrefix + "dlb_error_packets", dlbErrorPackets);
}

TeFlowStats SwSwitch::getTeFlowStats() {
  TeFlowStats teFlowStats;
  std::map<std::string, HwTeFlowStats> hwTeFlowStats;
  auto statMap = facebook::fb303::fbData->getStatMap();
  auto state = getState();
  if (!state) {
    return teFlowStats;
  }
  auto multiTeFlowTable = state->getTeFlowTable();
  for (const auto& [_, teFlowTable] : std::as_const(*multiTeFlowTable)) {
    for (const auto& [flowStr, flowEntry] : std::as_const(*teFlowTable)) {
      std::ignore = flowStr;
      if (flowEntry->getCounterID().has_value()) {
        const auto& counter = flowEntry->getCounterID();
        auto statName = folly::to<std::string>(counter->toThrift(), ".bytes");
        // returns default stat if statName does not exists
        auto statPtr = statMap->getStatPtrNoExport(statName);
        auto lockedStatPtr = statPtr->lock();
        auto numLevels = lockedStatPtr->numLevels();
        // Cumulative (ALLTIME) counters are at (numLevels - 1)
        HwTeFlowStats flowStat;
        flowStat.bytes() = lockedStatPtr->sum(numLevels - 1);
        hwTeFlowStats.emplace(counter->toThrift(), std::move(flowStat));
      }
    }
  }
  auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
  teFlowStats.timestamp() = now.count();
  teFlowStats.hwTeFlowStats() = std::move(hwTeFlowStats);
  return teFlowStats;
}

void SwSwitch::registerNeighborListener(
    std::function<void(
        const std::vector<std::string>& added,
        const std::vector<std::string>& deleted)> callback) {
  XLOG(DBG2) << "Registering neighbor listener";
  lock_guard<mutex> g(neighborListenerMutex_);
  neighborListener_ = std::move(callback);
}

void SwSwitch::invokeNeighborListener(
    const std::vector<std::string>& added,
    const std::vector<std::string>& removed) {
  lock_guard<mutex> g(neighborListenerMutex_);
  if (neighborListener_ && !isExiting()) {
    neighborListener_(added, removed);
  }
}

void SwSwitch::exitFatal() const noexcept {
  folly::dynamic switchState = folly::dynamic::object;
  // No hwswitch dump for multi swagent exit
  if (isRunModeMonolithic()) {
    switchState[kHwSwitch] = getMonolithicHwSwitchHandler()->toFollyDynamic();
  }
  state::WarmbootState thriftSwitchState;
  *thriftSwitchState.swSwitchState() = getAppliedState()->toThrift();
  if (!dumpStateToFile(agentDirUtil_->getCrashSwitchStateFile(), switchState) ||
      !dumpBinaryThriftToFile(
          agentDirUtil_->getCrashThriftSwitchStateFile(), thriftSwitchState)) {
    XLOG(ERR) << "Unable to write switch state JSON or Thrift to file";
  }
}

void SwSwitch::publishRxPacket(RxPacket* pkt, uint16_t ethertype) {
  RxPacketData pubPkt;
  pubPkt.srcPort = pkt->getSrcPort();
  pubPkt.srcVlan = pkt->getSrcVlanIf();

  for (const auto& r : pkt->getReasons()) {
    RxReason reason;
    reason.bytes = r.bytes;
    reason.description = r.description;
    pubPkt.reasons.push_back(reason);
  }

  folly::IOBuf buf_copy;
  pkt->buf()->cloneInto(buf_copy);
  pubPkt.packetData = buf_copy.moveToFbString();
}

void SwSwitch::publishTxPacket(TxPacket* pkt, uint16_t ethertype) {
  TxPacketData pubPkt;
  folly::IOBuf copy_buf;
  pkt->buf()->cloneInto(copy_buf);
  pubPkt.packetData = copy_buf.moveToFbString();
}

std::shared_ptr<SwitchState> SwSwitch::preInit(SwitchFlags flags) {
  auto begin = steady_clock::now();
  flags_ = flags;
  bootType_ = swSwitchWarmbootHelper_->canWarmBoot() ? BootType::WARM_BOOT
                                                     : BootType::COLD_BOOT;
  XLOG(INFO) << kNetworkEventLogPrefix
             << " Boot Type: " << apache::thrift::util::enumNameSafe(bootType_)
             << " | SDK version: " << getAsicSdkVersion(sdkVersion_)
             << " | Agent version: " << getBuildPackageVersion();

  swSwitchWarmbootHelper_->logBoot(
      apache::thrift::util::enumNameSafe(bootType_),
      getAsicSdkVersion(sdkVersion_),
      getBuildPackageVersion());

  multiHwSwitchHandler_->start();
  std::optional<state::WarmbootState> wbState{};
  if (bootType_ == BootType::WARM_BOOT) {
    wbState = swSwitchWarmbootHelper_->getWarmBootState();
  }
  restart_time::init(
      agentDirUtil_->getWarmBootDir(), bootType_ == BootType::WARM_BOOT);

  auto [state, rib] = SwSwitchWarmBootHelper::reconstructStateAndRib(
      wbState, scopeResolver_->hasL3());
  rib_ = std::move(rib);

  if (bootType_ != BootType::WARM_BOOT) {
    state = setupMinAlpmRouteState(*scopeResolver_, state);
    // rib should also have minimum alpm state
    rib_ = RoutingInformationBase::fromThrift(
        rib_->toThrift(),
        state->getFibs(),
        state->getLabelForwardingInformationBase());
  }

  fb303::fbData->setCounter(kHwUpdateFailures, 0);

  startThreads();

  // start lagMananger
  if (flags_ & SwitchFlags::ENABLE_LACP) {
    lagManager_ = std::make_unique<LinkAggregationManager>(this);
  }

  XLOG(DBG2)
      << "Time to init switch and start all threads "
      << duration_cast<duration<float>>(steady_clock::now() - begin).count();

  return state;
}

void SwSwitch::init(
    HwSwitchCallback* callback,
    std::unique_ptr<TunManager> tunMgr,
    HwSwitchInitFn hwSwitchInitFn,
    const HwWriteBehavior& hwWriteBehavior,
    SwitchFlags flags) {
  auto begin = steady_clock::now();
  flags_ = flags;
  auto failHwCallsOnWarmboot = (hwWriteBehavior == HwWriteBehavior::FAIL);
  auto hwInitRet = hwSwitchInitFn(callback, failHwCallsOnWarmboot);
  auto initialState = preInit(flags);
  if (hwInitRet.bootType != bootType_) {
    // this is being done for preprod2trunk migration. further until tooling
    // is updated to affect both warm boot flags, HwSwitch will override
    // SwSwitch boot flag (for monolithic agent).
    XLOG(INFO) << "Overriding boot type from: "
               << apache::thrift::util::enumNameSafe(bootType_) << " to "
               << apache::thrift::util::enumNameSafe(hwInitRet.bootType);

    bootType_ = hwInitRet.bootType;
  }
  if (getAppliedState()) {
    // applied state is already seeded by test
    initialState = getAppliedState();
  }
  initialState->publish();
  auto emptyState = std::make_shared<SwitchState>();
  emptyState->publish();
  const auto initialStateDelta = StateDelta(emptyState, initialState);

  // Notify resource accountant of the initial state.
  if (!resourceAccountant_->isValidRouteUpdate(initialStateDelta)) {
    // If DLB is enabled and pre-warmboot state has >128 ECMP groups, any
    // failure is due to DLB resource check failure. Resource accounting will
    // not be enabled in this boot and stay disabled until next warmboot
    //
    // This is the first invocation of isValidRouteUpdate. At this time,
    // ResourceAccountant::checkDlbResource_ is True by default. If the method
    // returns False, set checkDlbResource_ to False. This will disable further
    // DLB resource checks within resource accounting
    if (FLAGS_dlbResourceCheckEnable && FLAGS_flowletSwitchingEnable) {
      XLOG(DBG0) << "DLB resource check disabled until next warmboot";
      resourceAccountant_->enableDlbResourceCheck(false);
    } else {
      throw FbossError(
          "Not enough resource to apply initialState. ",
          "This should not happen given the state was previously applied, ",
          "but possible if calculation or threshold changes across warmboot.");
    }
  }
  multiHwSwitchHandler_->stateChanged(
      initialStateDelta, false, hwWriteBehavior);
  // For cold boot there will be discripancy between applied state and state
  // that exists in hardware. this discrepancy is until config is applied, after
  // that the two states are in sync. tolerating this discrepancy for now
  setStateInternal(initialState);

  XLOG(DBG0) << "hardware initialized in " << hwInitRet.bootTime
             << " seconds; applying initial config";

  if (flags & SwitchFlags::ENABLE_TUN) {
    // for monolithic agent enable tun manager early to retain existing behavior
    if (tunMgr) {
      tunMgr_ = std::move(tunMgr);
    } else {
      tunMgr_ = std::make_unique<TunManager>(this, &packetTxEventBase_);
    }
    tunMgr_->probe();
  }

  if (isRunModeMonolithic()) {
    getMonolithicHwSwitchHandler()->onHwInitialized(this);
  }

  // Notify the state observers of the initial state
  updateEventBase_.runInFbossEventBaseThread([initialState, this]() {
    notifyStateObservers(
        StateDelta(std::make_shared<SwitchState>(), initialState));
  });

  XLOG(DBG2)
      << "Time to init switch and start all threads and apply the state"
      << duration_cast<duration<float>>(steady_clock::now() - begin).count();

  postInit();

  if (scopeResolver_->hasL3()) {
    SwSwitchRouteUpdateWrapper(this, rib_.get()).programMinAlpmState();
  }
  if (FLAGS_log_all_fib_updates) {
    constexpr auto kAllFibUpdates = "all_fib_updates";
    logRouteUpdates("::", 0, kAllFibUpdates);
    logRouteUpdates("0.0.0.0", 0, kAllFibUpdates);
  }
}

void SwSwitch::init(
    std::unique_ptr<TunManager> tunMgr,
    HwSwitchInitFn hwSwitchInitFn,
    SwitchFlags flags) {
  this->init(
      this, std::move(tunMgr), hwSwitchInitFn, HwWriteBehavior::WRITE, flags);
}

void SwSwitch::init(const HwWriteBehavior& hwWriteBehavior, SwitchFlags flags) {
  /* used for split Software Switch init */
  auto initialState = preInit(flags);
  initialState->publish();
  // wait for HwSwitch connect
  std::shared_ptr<SwitchState> emptyState = std::make_shared<SwitchState>();
  emptyState->publish();
  if (!getHwSwitchHandler()->waitUntilHwSwitchConnected()) {
    throw FbossError("Waiting for HwSwitch to be connected cancelled");
  }
  const auto initialStateDelta = StateDelta(emptyState, initialState);
  // Notify resource accountant of the initial state.
  if (!resourceAccountant_->isValidRouteUpdate(initialStateDelta)) {
    // If DLB is enabled and pre-warmboot state has >128 ECMP groups, any
    // failure is due to DLB resource check failure. Resource accounting will
    // not be enabled in this boot and stay disabled until next warmboot
    //
    // This is the first invocation of isValidRouteUpdate. At this time,
    // ResourceAccountant::checkDlbResource_ is True by default. If the method
    // returns False, set checkDlbResource_ to False. This will disable further
    // DLB resource checks within resource accounting
    if (FLAGS_dlbResourceCheckEnable && FLAGS_flowletSwitchingEnable) {
      XLOG(DBG0) << "DLB resource check disabled until next warmboot";
      resourceAccountant_->enableDlbResourceCheck(false);
    } else {
      throw FbossError(
          "Not enough resource to apply initialState. ",
          "This should not happen given the state was previously applied, ",
          "but possible if calculation or threshold changes across warmboot.");
    }
  }
  // Do not send cold boot state to hwswitch. This is to avoid
  // deleting any cold boot state entries that hwswitch has learned from sdk
  if (bootType_ == BootType::WARM_BOOT) {
    try {
      getHwSwitchHandler()->stateChanged(
          initialStateDelta, false, hwWriteBehavior);
    } catch (const std::exception& ex) {
      throw FbossError("Failed to sync initial state to HwSwitch: ", ex.what());
    }
  }
  // for cold boot discrepancy may exist between applied state in software
  // switch and state that already exist in hardware. this discrepancy is
  // until config is applied, after that the two states are in sync.
  // tolerating this discrepancy for now.
  setStateInternal(initialState);
  if (bootType_ == BootType::WARM_BOOT) {
    // Notify the state observers of the initial state
    updateEventBase_.runInFbossEventBaseThread(
        [emptyState, initialState, this]() {
          notifyStateObservers(StateDelta(emptyState, initialState));
        });
  }
  if (FLAGS_log_all_fib_updates) {
    constexpr auto kAllFibUpdates = "all_fib_updates";
    logRouteUpdates("::", 0, kAllFibUpdates);
    logRouteUpdates("0.0.0.0", 0, kAllFibUpdates);
  }
  postInit();
}

void SwSwitch::initialConfigApplied(const steady_clock::time_point& startTime) {
  setSwitchRunState(SwitchRunState::CONFIGURED);

  if (flags_ & SwitchFlags::ENABLE_TUN) {
    // skip if mock tun manager was set by tests or during monolithic agent
    // initialization.
    // this is created only for split software agent after config is applied
    if (!tunMgr_) {
      tunMgr_ = std::make_unique<TunManager>(this, &packetTxEventBase_);
      tunMgr_->probe();
    }
  }

  if (tunMgr_) {
    // We check for syncing tun interface only on state changes after the
    // initial configuration is applied. This is really a hack to get around
    // 2 issues
    // a) On warm boot the initial state constructed from warm boot cache
    // does not know of interface addresses. This means if we sync tun
    // interface on applying initial boot up state we would blow away tunnel
    // interferace addresses, causing connectivity disruption. Once t4155406
    // is fixed we should be able to remove this check. b) Even if we were
    // willing to live with the above, the TunManager code does not properly
    // track deleting of interface addresses, viz. when we delete a
    // interface's primary address, secondary addresses get blown away as
    // well. TunManager does not track this and tries to delete the
    // secondaries as well leading to errors, t4746261 is tracking this.
    tunMgr_->startObservingUpdates();

    // Perform initial sync of interfaces
    tunMgr_->forceInitialSync();
  }

  if (lldpManager_) {
    lldpManager_->start();
  }

  if (flags_ & SwitchFlags::PUBLISH_STATS) {
    stats()->switchConfiguredMs(duration_cast<std::chrono::milliseconds>(
                                    steady_clock::now() - startTime)
                                    .count());
  }
#if FOLLY_HAS_COROUTINES
  if (flags_ & SwitchFlags::ENABLE_MACSEC) {
    mkaServiceManager_ = std::make_unique<MKAServiceManager>(this);
  }
#endif
  // Start FSDB state syncer post initial config applied. FSDB state
  // syncer will do a full sync upon connection establishment to FSDB
  runFsdbSyncFunction([](auto& syncer) { syncer->start(); });
}

void SwSwitch::logRouteUpdates(
    const std::string& addr,
    uint8_t mask,
    const std::string& identifier) {
  RouteUpdateLoggingInstance r{
      RoutePrefix<folly::IPAddress>{folly::IPAddress{addr}, mask},
      identifier,
      false};
  routeUpdateLogger_->startLoggingForPrefix(r);
}

void SwSwitch::stopLoggingRouteUpdates(const std::string& identifier) {
  routeUpdateLogger_->stopLoggingForIdentifier(identifier);
}

void SwSwitch::registerStateObserver(
    StateObserver* observer,
    const string& name) {
  XLOG(DBG2) << "Registering state observer: " << name;
  updateEventBase_.runImmediatelyOrRunInFbossEventBaseThreadAndWait(
      [=, this]() { addStateObserver(observer, name); });
}

void SwSwitch::unregisterStateObserver(StateObserver* observer) {
  updateEventBase_.runImmediatelyOrRunInFbossEventBaseThreadAndWait(
      [=, this]() { removeStateObserver(observer); });
}

bool SwSwitch::stateObserverRegistered(StateObserver* observer) {
  DCHECK(updateEventBase_.isInEventBaseThread());
  return stateObservers_.find(observer) != stateObservers_.end();
}

void SwSwitch::removeStateObserver(StateObserver* observer) {
  DCHECK(updateEventBase_.isInEventBaseThread());
  auto nErased = stateObservers_.erase(observer);
  if (!nErased) {
    throw FbossError("State observer remove failed: observer does not exist");
  }
}

void SwSwitch::addStateObserver(StateObserver* observer, const string& name) {
  DCHECK(updateEventBase_.isInEventBaseThread());
  if (stateObserverRegistered(observer)) {
    throw FbossError("State observer add failed: ", name, " already exists");
  }
  stateObservers_.emplace(observer, name);
}

void SwSwitch::notifyStateObservers(const StateDelta& delta) {
  CHECK(updateEventBase_.inRunningEventBaseThread());
  if (isExiting()) {
    // Make sure the SwSwitch is not already being destroyed
    return;
  }
  // Update AddrToLocalIntf map maintained in sw switch, for fast interface
  // lookup in rx path.
  updateAddrToLocalIntf(delta);

  for (auto observerName : stateObservers_) {
    try {
      auto observer = observerName.first;
      observer->stateUpdated(delta);
    } catch (const std::exception& ex) {
      // TODO: Figure out the best way to handle errors here.
      XLOG(FATAL) << "error notifying " << observerName.second
                  << " of update: " << folly::exceptionStr(ex);
    }
  }
  runFsdbSyncFunction([&delta](auto& syncer) { syncer->stateUpdated(delta); });
}

template <typename FsdbFunc>
void SwSwitch::runFsdbSyncFunction(FsdbFunc&& fn) {
  fsdbSyncer_.withWLock([&](auto& syncer) {
    if (syncer) {
      fn(syncer);
    }
  });
}

bool SwSwitch::updateState(unique_ptr<StateUpdate> update) {
  if (isExiting()) {
    XLOG(DBG2) << " Skipped queuing update: " << update->getName()
               << " since exit already started";
    return false;
  }
  {
    std::unique_lock guard(pendingUpdatesLock_);
    pendingUpdates_.push_back(*update.release());
  }

  // Signal the update thread that updates are pending.
  // We call runInFbossEventBaseThread() with a static function pointer since
  // this is more efficient than having to allocate a new bound function object.
  updateEventBase_.runInFbossEventBaseThread(handlePendingUpdatesHelper, this);
  return true;
}

bool SwSwitch::updateState(StringPiece name, StateUpdateFn fn) {
  auto update = make_unique<FunctionStateUpdate>(name, std::move(fn));
  return updateState(std::move(update));
}

void SwSwitch::updateStateNoCoalescing(StringPiece name, StateUpdateFn fn) {
  auto update = make_unique<FunctionStateUpdate>(
      name,
      std::move(fn),
      static_cast<int>(StateUpdate::BehaviorFlags::NON_COALESCING));
  updateState(std::move(update));
}

void SwSwitch::updateStateBlocking(folly::StringPiece name, StateUpdateFn fn) {
  auto behaviorFlags = static_cast<int>(StateUpdate::BehaviorFlags::NONE);
  updateStateBlockingImpl(name, fn, behaviorFlags);
}

void SwSwitch::updateStateWithHwFailureProtection(
    folly::StringPiece name,
    StateUpdateFn fn) {
  int stateUpdateBehavior =
      static_cast<int>(StateUpdate::BehaviorFlags::NON_COALESCING) |
      static_cast<int>(StateUpdate::BehaviorFlags::HW_FAILURE_PROTECTION);

  updateStateBlockingImpl(name, fn, stateUpdateBehavior);
}

void SwSwitch::updateStateBlockingImpl(
    folly::StringPiece name,
    StateUpdateFn fn,
    int stateUpdateBehavior) {
  auto result = std::make_shared<BlockingUpdateResult>();
  auto update = make_unique<BlockingStateUpdate>(
      name, std::move(fn), result, stateUpdateBehavior);
  if (updateState(std::move(update))) {
    result->wait();
  }
}

void SwSwitch::handlePendingUpdatesHelper(SwSwitch* sw) {
  sw->handlePendingUpdates();
}

void SwSwitch::handlePendingUpdates() {
  // Get the list of updates to run.
  //
  // We might pull multiple updates off the list at once if several updates
  // were scheduled before we had a chance to process them.  In some cases we
  // might also end up finding 0 updates to process if a previous
  // handlePendingUpdates() call processed multiple updates.
  StateUpdateList updates;
  {
    std::unique_lock guard(pendingUpdatesLock_);
    // When deciding how many elements to pull off the pendingUpdates_
    // list, we pull as many as we can, subject to the following conditions
    // - Non coalescing updates are executed by themselves
    auto iter = pendingUpdates_.begin();
    while (iter != pendingUpdates_.end()) {
      StateUpdate* update = &(*iter);
      if (update->isNonCoalescing()) {
        if (iter == pendingUpdates_.begin()) {
          // First update is non coalescing, splice it onto the updates list
          // and apply transaction by itself
          ++iter;
          break;
        } else {
          // Splice all updates upto this non coalescing update, we will
          // get the non coalescing update in the next round
          break;
        }
      }
      ++iter;
    }
    updates.splice(
        updates.begin(), pendingUpdates_, pendingUpdates_.begin(), iter);
  }

  // handlePendingUpdates() is invoked once for each update, but a previous
  // call might have already processed everything.  If we don't have anything
  // to do just return early.
  if (updates.empty()) {
    return;
  }

  // Non coalescing updates should be applied individually
  bool isNonCoalescing = updates.begin()->isNonCoalescing();
  if (isNonCoalescing) {
    CHECK_EQ(updates.size(), 1)
        << " Non coalescing updates should be applied individually";
  }
  if (updates.begin()->hwFailureProtected()) {
    CHECK(isNonCoalescing)
        << " Hw Failure protected updates should be non coalescing";
  }

  // This function should never be called with valid updates while we don't have
  // a valid switch state
  DCHECK(getState());

  // Call all of the update functions to prepare the new SwitchState
  auto oldAppliedState = getState();
  // We start with the old state, and apply state updates one at a time.
  auto newDesiredState = oldAppliedState;
  auto iter = updates.begin();
  while (iter != updates.end()) {
    StateUpdate* update = &(*iter);
    ++iter;

    shared_ptr<SwitchState> intermediateState;
    XLOG(DBG2) << "preparing state update " << update->getName();
    try {
      intermediateState = update->applyUpdate(newDesiredState);
    } catch (const std::exception& ex) {
      // Call the update's onError() function, and then immediately delete
      // it (therefore removing it from the intrusive list).  This way we
      // won't call it's onSuccess() function later.
      update->onError(ex);
      delete update;
    }
    // We have applied the update to software switch state, so call success
    // on the update.
    if (intermediateState) {
      // Call publish after applying each StateUpdate.  This guarantees that
      // the next StateUpdate function will have clone the SwitchState before
      // making any changes.  This ensures that if a StateUpdate function
      // ever fails partway through it can't have partially modified our
      // existing state, leaving it in an invalid state.
      intermediateState->publish();
      newDesiredState = intermediateState;
    }
  }
  // Start newAppliedState as equal to newDesiredState unless
  // we learn otherwise
  auto newAppliedState = newDesiredState;
  // Now apply the update and notify subscribers
  if (newDesiredState != oldAppliedState) {
    auto isTransaction = updates.begin()->hwFailureProtected() &&
        multiHwSwitchHandler_->transactionsSupported();
    // There was some change during these state updates
    newAppliedState =
        applyUpdate(oldAppliedState, newDesiredState, isTransaction);
    if (newDesiredState != newAppliedState) {
      if (isExiting()) {
        /*
         * If we started exit, applyUpdate will reject updates leading
         * to a mismatch b/w applied and desired states. Log, but otherwise
         * ignore this error. Ideally, we should throw this error back to
         * the callers, but during exit with threads in various state of
         * stoppage, it becomes hard to manage. Since we require a resync
         * of state post restart anyways (through WB state replay, config
         * application and FIB sync) this should not cause problems. Note
         * that this sync is not optional, but required for external clients
         * post restart - agent could COLD boot, or there could be a mismatch
         * with the HW state.
         * If we ever want to relax this requirement - viz. only require a
         * resync from external clients on COLD boot, we will need to get
         * more rigorous here.
         */
        XLOG(DBG2) << " Failed to apply updates to HW since SwSwtich already "
                      "started exit";
      } else if (updates.size() == 1 && updates.begin()->hwFailureProtected()) {
        fb303::fbData->incrementCounter(kHwUpdateFailures);
        unique_ptr<StateUpdate> update(&updates.front());
        try {
          throw FbossHwUpdateError(
              newDesiredState,
              newAppliedState,
              "Update : ",
              update->getName(),
              " application to HW failed");

        } catch (const std::exception& ex) {
          update->onError(ex);
        }
        return;
      } else {
        XLOG(FATAL)
            << " Failed to apply update to HW and the update is not marked for "
               "HW failure protection";
      }
    } else {
      // Update successful, update hw update counter to zero.
      fb303::fbData->setCounter(kHwUpdateFailures, 0);
    }
  }
  updatePtpTcCounter();
  // Notify all of the updates of success and delete them.
  while (!updates.empty()) {
    unique_ptr<StateUpdate> update(&updates.front());
    updates.pop_front();
    update->onSuccess();
  }
}

void SwSwitch::updatePtpTcCounter() {
  // update fb303 counter to reflect current state of PTP
  // should be invoked post update
  auto switchSettings = utility::getFirstNodeIf(getState()->getSwitchSettings())
      ? utility::getFirstNodeIf(getState()->getSwitchSettings())
      : std::make_shared<SwitchSettings>();
  fb303::fbData->setCounter(
      SwitchStats::kCounterPrefix + "ptp_tc_enabled",
      switchSettings->isPtpTcEnable() ? 1 : 0);
}

void SwSwitch::setStateInternal(std::shared_ptr<SwitchState> newAppliedState) {
  // This is one of the only two places that should ever directly access
  // stateDontUseDirectly_.  (getState() being the other one.)
  CHECK(bool(newAppliedState));
  CHECK(newAppliedState->isPublished());
  std::unique_lock guard(stateLock_);
  appliedStateDontUseDirectly_.swap(newAppliedState);
}

std::shared_ptr<SwitchState> SwSwitch::applyUpdate(
    const shared_ptr<SwitchState>& oldState,
    const shared_ptr<SwitchState>& newState,
    bool isTransaction) {
  // Check that we are starting from what has been already applied
  DCHECK_EQ(oldState, getAppliedState());

  auto start = std::chrono::steady_clock::now();
  XLOG(DBG2) << "Updating state: old_gen=" << oldState->getGeneration()
             << " new_gen=" << newState->getGeneration();
  DCHECK_GT(newState->getGeneration(), oldState->getGeneration());

  StateDelta delta(oldState, newState);

  // If we are already exiting, abort the update
  if (isExiting()) {
    XLOG(DBG2) << " Agent exiting before all updates could be applied";
    return oldState;
  }

  if (!resourceAccountant_->isValidUpdate(delta)) {
    // Notify resource account to revert back to previous state
    resourceAccountant_->stateChanged(StateDelta(newState, oldState));
    return oldState;
  }

  std::shared_ptr<SwitchState> newAppliedState;

  // Inform the HwSwitch of the change.
  //
  // Note that at this point we have already updated the state pointer and
  // released stateLock_, so the new state is already published and visible to
  // other threads.  This does mean that there is a window where the new state
  // is visible but the hardware is not using the new configuration yet.
  //
  // We could avoid this by holding a lock and block anyone from reading the
  // state while we update the hardware.  However, updating the hardware may
  // take a non-trivial amount of time, and blocking other users seems
  // undesirable.  So far I don't think this brief discrepancy should cause
  // major issues.
  try {
    newAppliedState = stateChanged(delta, isTransaction);
  } catch (const std::exception& ex) {
    // Notify the hw_ of the crash so it can execute any device specific
    // tasks before we fatal. An example would be to dump the current hw
    // state.
    //
    // Another thing we could try here is rolling back to the old state.
    XLOG(ERR) << "error applying state change to hardware: "
              << folly::exceptionStr(ex);

    if (isRunModeMonolithic()) {
      getMonolithicHwSwitchHandler()->exitFatal();
    }

    dumpBadStateUpdate(oldState, newState);
    XLOG(FATAL) << "encountered a fatal error: " << folly::exceptionStr(ex);
  }

  setStateInternal(newAppliedState);

  // Notifies all observers of the current state update.
  notifyStateObservers(StateDelta(oldState, newAppliedState));

  // Notifies resource accountant of new applied state.
  resourceAccountant_->stateChanged(StateDelta(newState, newAppliedState));

  auto end = std::chrono::steady_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  stats()->stateUpdate(duration);

  XLOG(DBG0) << "Update state took " << duration.count() << "us";
  return newAppliedState;
}

void SwSwitch::dumpBadStateUpdate(
    const std::shared_ptr<SwitchState>& oldState,
    const std::shared_ptr<SwitchState>& newState) const {
  // dump the previous state and target state to understand what led to the
  // crash
  utilCreateDir(agentDirUtil_->getCrashBadStateUpdateDir());
  if (!dumpBinaryThriftToFile(
          agentDirUtil_->getCrashBadStateUpdateOldStateFile(),
          oldState->toThrift())) {
    XLOG(ERR) << "Unable to write old switch state thrift to "
              << agentDirUtil_->getCrashBadStateUpdateOldStateFile();
  }
  if (!dumpBinaryThriftToFile(
          agentDirUtil_->getCrashBadStateUpdateNewStateFile(),
          newState->toThrift())) {
    XLOG(ERR) << "Unable to write new switch state thrift to "
              << agentDirUtil_->getCrashBadStateUpdateNewStateFile();
  }
}

std::map<PortID, const phy::PhyInfo> SwSwitch::getIPhyInfo(
    std::vector<PortID>& portIDs) {
  return phySnapshotManager_->getPhyInfos(portIDs);
}

PortStats* SwSwitch::portStats(PortID portID) {
  auto portStats = stats()->port(portID);
  if (portStats) {
    return portStats;
  }
  auto portIf = getState()->getPorts()->getNodeIf(portID);
  if (portIf) {
    // get portName from current state
    return stats()->createPortStats(
        portID, getState()->getPort(portID)->getName());
  } else {
    // only for port0 case
    XLOG(DBG0) << "Port node doesn't exist, use default name=port" << portID;
    return stats()->createPortStats(portID, folly::to<string>("port", portID));
  }
}

PortStats* SwSwitch::portStats(const RxPacket* pkt) {
  return portStats(pkt->getSrcPort());
}

InterfaceStats* SwSwitch::interfaceStats(InterfaceID intfID) {
  auto intfStats = stats()->intf(intfID);
  if (intfStats) {
    return intfStats;
  }
  auto interfaceIf = getState()->getInterfaces()->getNodeIf(intfID);
  if (!interfaceIf) {
    XLOG(DBG0) << "Interface node doesn't exist, use default name=intf"
               << intfID;
    return stats()->createInterfaceStats(
        intfID, folly::to<string>("intf", intfID));
  }
  return stats()->createInterfaceStats(intfID, interfaceIf->getName());
}

map<int32_t, PortStatus> SwSwitch::getPortStatus() {
  map<int32_t, PortStatus> statusMap;
  std::shared_ptr<MultiSwitchPortMap> portMaps = getState()->getPorts();
  for (const auto& portMap : std::as_const(*portMaps)) {
    for (const auto& p : std::as_const(*portMap.second)) {
      statusMap[p.second->getID()] = fillInPortStatus(*p.second, this);
    }
  }
  return statusMap;
}

PortStatus SwSwitch::getPortStatus(PortID portID) {
  std::shared_ptr<Port> port = getState()->getPort(portID);
  return fillInPortStatus(*port, this);
}

SwitchStats* SwSwitch::createSwitchStats() {
  CHECK(switchInfoTable_.getSwitchIdToSwitchInfo().size());
  SwitchStats* s =
      new SwitchStats(switchInfoTable_.getSwitchIdToSwitchInfo().size());
  stats_.reset(s);
  return s;
}

void SwSwitch::setPortStatusCounter(PortID port, bool up) {
  auto state = getState();
  if (!state) {
    // Make sure the state actually exists, this could be an issue if
    // called during initialization
    return;
  }
  portStats(port)->setPortStatus(up);
}

void SwSwitch::setPortActiveStatusCounter(PortID port, bool isActive) {
  auto state = getState();
  if (!state) {
    // Make sure the state actually exists, this could be an issue if
    // called during initialization
    return;
  }
  portStats(port)->setPortActiveStatus(isActive);
}

void SwSwitch::packetReceived(std::unique_ptr<RxPacket> pkt) noexcept {
  PortID port = pkt->getSrcPort();
  try {
    auto now = steady_clock::now();
    auto lastTime = lastPacketRxTime_.load();
    if (lastTime != std::chrono::steady_clock::time_point::min()) {
      auto delay = duration_cast<milliseconds>(now - lastPacketRxTime_.load());
      stats()->packetRxHeartbeatDelay(delay.count());
    }
    lastPacketRxTime_ = now;
    handlePacket(std::move(pkt));
  } catch (const std::exception& ex) {
    portStats(port)->pktError();
    XLOG(ERR) << "error processing trapped packet: " << folly::exceptionStr(ex)
              << " from port: " << port;
    // Return normally, without letting the exception propagate to our caller.
    return;
  }
}

void SwSwitch::packetReceivedThrowExceptionOnError(
    std::unique_ptr<RxPacket> pkt) {
  handlePacket(std::move(pkt));
}

PortDescriptor SwSwitch::getPortFromPkt(const RxPacket* pkt) const {
  if (pkt->isFromAggregatePort()) {
    return PortDescriptor(AggregatePortID(pkt->getSrcAggregatePort()));
  } else {
    return PortDescriptor(PortID(pkt->getSrcPort()));
  }
}

void SwSwitch::handlePacket(std::unique_ptr<RxPacket> pkt) {
  if (FLAGS_intf_nbr_tables) {
    auto intf = getState()->getInterfaces()->getNodeIf(
        getState()->getInterfaceIDForPort(getPortFromPkt(pkt.get())));
    handlePacketImpl(std::move(pkt), intf);
  } else {
    auto vlan =
        getState()->getVlans()->getNodeIf(getVlanIDHelper(pkt->getSrcVlanIf()));
    handlePacketImpl(std::move(pkt), vlan);
  }
}

template <typename VlanOrIntfT>
void SwSwitch::handlePacketImpl(
    std::unique_ptr<RxPacket> pkt,
    const std::shared_ptr<VlanOrIntfT>& vlanOrIntf) {
  // If we are not fully initialized or are already exiting, don't handle
  // packets since the individual handlers, h/w sdk data structures
  // may not be ready or may already be (partially) destroyed
  if (!isFullyInitialized()) {
    XLOG(DBG3) << "Dropping received packets received on UNINITIALIZED switch";
    return;
  }
  PortID port = pkt->getSrcPort();
  portStats(port)->trappedPkt();

  pktObservers_->packetReceived(pkt.get());

  /*
   * The minimum required frame length for ethernet is 64 bytes.
   * Abort processing early if the packet is too short.
   * Adding an option to override this minimum length in case
   * wedge_agent is used with simulator. In this case, wedge_agent
   * using simulator can be connected to a peer linux_agent running
   * in a container with a GRE tunnel interconnecting them. An ARP
   * request/response from linux_agent will not be padded to be
   * 64B coz of the GRE header added, but as GRE tunnel is decaped
   * before the ARP packet is handed over to wedge_agent, the
   * received frame size as seen by wedge_agent will be <64B.
   */
  auto len = pkt->getLength();
  if (len < FLAGS_minimum_ethernet_packet_length) {
    portStats(port)->pktBogus();
    return;
  }

  // Parse the source and destination MAC, as well as the ethertype.
  Cursor c(pkt->buf());
  auto dstMac = PktUtil::readMac(&c);
  auto srcMac = PktUtil::readMac(&c);
  auto ethertype = c.readBE<uint16_t>();
  if (ethertype == 0x8100) {
    // 802.1Q
    c += 2; // Advance over the VLAN tag.  We ignore it for now
    ethertype = c.readBE<uint16_t>();
  }

  auto vlanID = getVlanIDFromVlanOrIntf(vlanOrIntf);
  auto vlanIDStr = vlanID.has_value()
      ? folly::to<std::string>(static_cast<int>(vlanID.value()))
      : "None";

  XLOG(DBG5) << "trapped packet: src_port=" << pkt->getSrcPort()
             << " srcAggPort="
             << (pkt->isFromAggregatePort()
                     ? folly::to<string>(pkt->getSrcAggregatePort())
                     : "None")
             << " vlan=" << vlanIDStr << " length=" << len << " src=" << srcMac
             << " dst=" << dstMac << " ethertype=0x" << std::hex << ethertype
             << " :: " << pkt->describeDetails();
  XLOG_EVERY_N(DBG2, 10000)
      << "sampled " << "trapped packet: src_port=" << pkt->getSrcPort()
      << " srcAggPort="
      << (pkt->isFromAggregatePort()
              ? folly::to<string>(pkt->getSrcAggregatePort())
              : "None")
      << " vlan=" << vlanIDStr << " length=" << len << " src=" << srcMac
      << " dst=" << dstMac << " ethertype=0x" << std::hex << ethertype
      << " :: " << pkt->describeDetails();

  switch (ethertype) {
    case ArpHandler::ETHERTYPE_ARP:
      arp_->handlePacket(std::move(pkt), dstMac, srcMac, c, vlanOrIntf);
      return;
    case LldpManager::ETHERTYPE_LLDP:
      if (lldpManager_) {
        lldpManager_->handlePacket(std::move(pkt), dstMac, srcMac, c);
        return;
      }
      break;
#if FOLLY_HAS_COROUTINES
    case MKAServiceManager::ETHERTYPE_EAPOL:
      if (mkaServiceManager_) {
        portStats(port)->MkPduRecvdPkt();
        mkaServiceManager_->handlePacket(std::move(pkt));
        return;
      }
      break;
#endif
    case IPv4Handler::ETHERTYPE_IPV4:
      ipv4_->handlePacket(std::move(pkt), dstMac, srcMac, c, vlanOrIntf);
      return;
    case IPv6Handler::ETHERTYPE_IPV6:
      ipv6_->handlePacket(std::move(pkt), dstMac, srcMac, c, vlanOrIntf);
      return;
    case LACPDU::EtherType::SLOW_PROTOCOLS: {
      // The only supported protocol in the Ethernet suite's "Slow Protocols"
      // is the Link Aggregation Control Protocol
      auto subtype = c.readBE<uint8_t>();
      if (subtype == LACPDU::EtherSubtype::LACP) {
        if (lagManager_) {
          lagManager_->handlePacket(std::move(pkt), c);
        } else {
          LOG_EVERY_N(WARNING, 60)
              << "Received LACP frame but LACP not enabled";
        }
      } else if (subtype == LACPDU::EtherSubtype::MARKER) {
        XLOG(WARNING)
            << "Received Marker frame but Marker Protocol not supported";
      }
    }
      return;
    case (uint16_t)ETHERTYPE::ETHERTYPE_MPLS: {
      // updates the cursor
      auto hdr = MPLSHdr(&c);
      mplsHandler_->handlePacket(std::move(pkt), hdr, c);
      break;
    }
    default:
      break;
  }

  // If we are still here, we don't know what to do with this packet.
  // Increment a counter and just drop the packet on the floor.
  portStats(port)->pktUnhandled();
}

void SwSwitch::pfcWatchdogStateChanged(
    const PortID& portId,
    const bool deadlockDetected) {
  if (!isFullyInitialized()) {
    XLOG(ERR)
        << "Ignore PFC watchdog event changes before we are fully initialized...";
    return;
  }
  auto state = getState();
  if (!state) {
    // Make sure the state actually exists, this could be an issue if
    // called during initialization
    XLOG(ERR)
        << "Ignore PFC watchdog event changes, as switch state doesn't exist yet";
    return;
  }
  if (deadlockDetected) {
    portStats(portId)->pfcDeadlockDetectionCount();
  } else {
    portStats(portId)->pfcDeadlockRecoveryCount();
  }
}

void SwSwitch::linkStateChanged(
    PortID portId,
    bool up,
    cfg::PortType portType,
    std::optional<phy::LinkFaultStatus> iPhyFaultStatus) {
  if (!isFullyInitialized()) {
    XLOG(ERR)
        << "Ignore link state change event before we are fully initialized...";
    return;
  }

  // Schedule an update for port's operational status
  auto updateOperStateFn = [=,
                            this](const std::shared_ptr<SwitchState>& state) {
    std::shared_ptr<SwitchState> newState(state);
    auto* port = newState->getPorts()->getNodeIf(portId).get();

    if (port) {
      if (port->isUp() != up) {
        port = port->modify(&newState);
        port->setOperState(up);
        if (iPhyFaultStatus) {
          port->setIPhyLinkFaultStatus(*iPhyFaultStatus);
        }
        // Log event and update counters if there is a change
        logLinkStateEvent(portId, up);
        setPortStatusCounter(portId, up);
        std::optional<bool> portActive;
        if (port->getActiveState().has_value()) {
          portActive = *port->getActiveState() == Port::ActiveState::ACTIVE;
        }
        auto portSwitchId = scopeResolver_->scope(port->getID()).switchId();
        portStats(portId)->linkStateChange(
            up, isPortDrained(state, port, portSwitchId), portActive);

        XLOG(DBG2) << "SW Link state changed: " << port->getName()
                   << " id: " << portId << " [" << (!up ? "UP" : "DOWN") << "->"
                   << (up ? "UP" : "DOWN") << "]";
      }
    }

    return newState;
  };

  // Note: On NIF ports, we never coalesce link up/down updates to ensure
  // expiry of NDP/ARP entries, but that is not applicable for Fabric port.
  // Thus, coalesce updates on Fabric ports. This is especially useful for a
  // flapping fabric ports as coalescing means that fewer state updates get
  // queued and other state updates (e.g. drain request to take a flapping
  // link off the prod) can get through quicker.
  if (portType == cfg::PortType::FABRIC_PORT) {
    updateState(
        "Fabric Port OperState (UP/DOWN) Update", std::move(updateOperStateFn));
  } else {
    updateStateNoCoalescing(
        "Port OperState (UP/DOWN) Update", std::move(updateOperStateFn));
  }
}

void SwSwitch::linkActiveStateChangedOrFwIsolated(
    const std::map<PortID, bool>& port2IsActive,
    bool fwIsolated,
    const std::optional<uint32_t>& numActiveFabricPortsAtFwIsolate) {
  if (!isFullyInitialized()) {
    XLOG(ERR)
        << "Ignore link active state change event before we are fully initialized...";
    return;
  }

  auto updateActiveStateFn = [=,
                              this](const std::shared_ptr<SwitchState>& state) {
    std::shared_ptr<SwitchState> newState(state);
    if (port2IsActive.size() == 0) {
      return newState;
    }

    auto numActiveFabricPorts = 0;
    for (const auto& [portID, isActive] : port2IsActive) {
      auto* port = newState->getPorts()->getNodeIf(portID).get();
      if (port) {
        if (isActive) {
          numActiveFabricPorts++;
        }

        if (port->isActive() != isActive) {
          setPortActiveStatusCounter(portID, isActive);
          portStats(portID)->linkActiveStateChange(isActive);

          auto getActiveStr = [](std::optional<bool> isActive) {
            return isActive.has_value()
                ? (isActive.value() ? "ACTIVE" : "INACTIVE")
                : "NONE";
          };
          XLOG(DBG2) << "SW Link state changed: " << port->getName() << " ["
                     << getActiveStr(port->isActive()) << "->"
                     << getActiveStr(isActive) << "]";

          port = port->modify(&newState);
          port->setActiveState(isActive);
        }
      }
    }

    // Pick matcher for any port.
    // This is OK because the matcher is used to retrieve switchSettings which
    // are same for all the ports of a HwSwitch.
    // And, SwSwitch::linkActiveStateChanged is invoked by a HwSwitch and thus
    // passed port2IsActive always contains ports from a single HwSwitch.
    auto matcher = getScopeResolver()->scope(port2IsActive.cbegin()->first);
    auto switchSettings =
        state->getSwitchSettings()->getNodeIf(matcher.matcherString());

    SwitchDrainState newActualSwitchDrainState;
    if (fwIsolated) {
      if (isSwitchErrorFirmwareIsolate(
              numActiveFabricPortsAtFwIsolate, switchSettings)) {
        stats()->fwDrainedWithHighNumActiveFabricLinks();
        if (FLAGS_fw_drained_unrecoverable_error) {
          newActualSwitchDrainState =
              cfg::SwitchDrainState::DRAINED_DUE_TO_ASIC_ERROR;
        } else {
          newActualSwitchDrainState = cfg::SwitchDrainState::DRAINED;
        }
      } else {
        newActualSwitchDrainState = cfg::SwitchDrainState::DRAINED;
      }
    } else {
      newActualSwitchDrainState =
          computeActualSwitchDrainState(switchSettings, numActiveFabricPorts);
    }

    auto currentActualDrainState = switchSettings->getActualSwitchDrainState();

    if (newActualSwitchDrainState != currentActualDrainState) {
      stats()->setDrainState(matcher.switchId(), newActualSwitchDrainState);
      auto newSwitchSettings = switchSettings->modify(&newState);
      newSwitchSettings->setActualSwitchDrainState(newActualSwitchDrainState);
    }

    XLOG(DBG2) << "SwitchID: " << static_cast<int>(matcher.switchId())
               << " | FwIsolated: " << (fwIsolated ? "Y" : "N")
               << " | ActivePortsAtFwIsolate: "
               << (numActiveFabricPortsAtFwIsolate.has_value()
                       ? folly::to<std::string>(
                             numActiveFabricPortsAtFwIsolate.value())
                       : "--")
               << " | "
               << getDrainStateChangedStr(getState(), newState, matcher)
               << " | ActivePorts: " << numActiveFabricPorts << "/"
               << port2IsActive.size() << " ("
               << getDrainThresholdStr(
                      newActualSwitchDrainState, switchSettings.get())
               << ")";

    return newState;
  };

  /*
   * Consider the following scenario:
   *  - Continuous flaps on fabric port(s).
   *  - linkActiveStateChanged callbacks for each flap.
   *  - A large number of state updates will be queued.
   *  - If processed without coalescing, state update queue will build up.
   *  - This will significantly delay any other state updates.
   *  - For example, Self Healing's attempt to drain a flapping link will
   *    require a state update, and could get significantly delayed.
   *
   * Thus, process linkActiveStateChanged with coalescing.
   *
   * Note: On NIF ports, we never coalesce link up/down updates to ensure
   * expiry of NDP/ARP entries, but that is not applicable for Fabric port.
   *
   * But, there is an exception, consider the following scenario:
   *   - A large number of fabric links flap.
   *   - These links will transition ACTIVE => INACTIVE => ACTIVE.
   *   - It is possible that the Firmware may isolate the device.
   *   - Device is isolated, but SwitchState is UNDRAINED(unisolated)..(0)
   *   - Firmware cb handling will decide to DRAIN (isolate) the device..(1)
   *   - Switch active/inactive cb processing may decide to UNDRAIN.. (2)
   *   - If (1) and (2) are coalesced, SwitchState will remain UNDRAINED and
   *     the device will be isolated i.e. state (0), not what we want.
   *
   * Prevent this by not coalesing state update on Firmware Isolate.
   *
   * Firmware Isolate is a rare event, and thus it is OK to not coalesce these
   * updates.
   */
  if (!fwIsolated) {
    updateState(
        "Port ActiveState (ACTIVE/INACTIVE) Update",
        std::move(updateActiveStateFn));
  } else {
    updateStateNoCoalescing(
        "Fw Isolate Update", std::move(updateActiveStateFn));
  }
}

void SwSwitch::switchReachabilityChanged(
    const SwitchID switchId,
    const std::map<SwitchID, std::set<PortID>>& switchReachabilityInfo) {
  switch_reachability::SwitchReachability newReachability;
  int currentIdx = 1;
  std::unordered_map<std::set<PortID>, int> portGrp2Id;
  for (const auto& [destinationSwitchId, portIdSet] : switchReachabilityInfo) {
    int portGroupId;
    auto [_, inserted] = portGrp2Id.insert({portIdSet, currentIdx});
    if (inserted) {
      std::vector<std::string> portGroup(portIdSet.size());
      std::transform(
          portIdSet.begin(),
          portIdSet.end(),
          portGroup.begin(),
          [this](PortID portId) {
            return getState()->getPorts()->getNode(portId)->getName();
          });
      newReachability.fabricPortGroupMap()[currentIdx] = std::move(portGroup);
      portGroupId = currentIdx++;
    } else {
      // Need to find the ID for the portIdSet that was already added
      portGroupId = portGrp2Id.find(portIdSet)->second;
    }
    newReachability.switchIdToFabricPortGroupMap()[static_cast<int64_t>(
        destinationSwitchId)] = portGroupId;
  }
  // Update switch reachability info with the latest data
  (*hwSwitchReachability_.wlock())[switchId] = newReachability;
  runFsdbSyncFunction([switchId, &newReachability](auto& syncer) {
    syncer->switchReachabilityChanged(switchId, std::move(newReachability));
  });
  // Update processing complete counter
  stats()->switchReachabilityChangeProcessed();
}

void SwSwitch::packetRxThread() {
  packetRxRunning_.store(true);
  while (packetRxRunning_.load()) {
    {
      std::unique_lock<std::mutex> lk(rxPktMutex_);
      rxPktThreadCV_.wait_for(
          lk, std::chrono::milliseconds(FLAGS_rx_pkt_thread_timeout), [this] {
            return (
                rxPacketHandlerQueues_.hasPacketsToProcess() ||
                !packetRxRunning_.load());
          });
    }
    if (!packetRxRunning_.load()) {
      return;
    }
#if FOLLY_HAS_COROUTINES
    while (rxPacketHandlerQueues_.hasPacketsToProcess()) {
      if (!packetRxRunning_.load()) {
        return;
      }
      auto hiPriPkt = rxPacketHandlerQueues_.getHiPriRxPktQueue().try_dequeue();
      if (hiPriPkt) {
        this->packetReceived(std::move(*hiPriPkt));
        continue;
      }
      auto midPriPkt =
          rxPacketHandlerQueues_.getMidPriRxPktQueue().try_dequeue();
      if (midPriPkt) {
        this->packetReceived(std::move(*midPriPkt));
        continue;
      }
      auto loPriPkt = rxPacketHandlerQueues_.getLoPriRxPktQueue().try_dequeue();
      if (loPriPkt) {
        this->packetReceived(std::move(*loPriPkt));
        continue;
      }
    }
#endif
  }
}

void SwSwitch::startThreads() {
  backgroundThread_.reset(new std::thread(
      [this] { this->threadLoop("fbossBgThread", &backgroundEventBase_); }));
  updateThread_.reset(new std::thread(
      [this] { this->threadLoop("fbossUpdateThread", &updateEventBase_); }));
  packetTxThread_.reset(new std::thread(
      [this] { this->threadLoop("fbossPktTxThread", &packetTxEventBase_); }));
  pcapDistributionThread_.reset(new std::thread([this] {
    this->threadLoop(
        "fbossPcapDistributionThread", &pcapDistributionEventBase_);
  }));
  neighborCacheThread_.reset(new std::thread([this] {
    this->threadLoop("fbossNeighborCacheThread", &neighborCacheEventBase_);
  }));
  // start LACP thread, start before creating LinkAggregationManager
  lacpThread_.reset(new std::thread(
      [this] { this->threadLoop("fbossLacpThread", &lacpEventBase_); }));
  if (FLAGS_rx_sw_priority) {
    packetRxThread_.reset(new std::thread(
        [this] { this->threadLoop("fbossPktRxThread", &packetRxEventBase_); }));
    packetRxEventBase_.runInEventBaseThread([this] { this->packetRxThread(); });
  }
}

void SwSwitch::postInit() {
  if (flags_ & SwitchFlags::ENABLE_LLDP) {
    lldpManager_ = std::make_unique<LldpManager>(this);
  }

  if (flags_ & SwitchFlags::PUBLISH_STATS) {
    switch (bootType_) {
      case BootType::COLD_BOOT:
        stats()->coldBoot();
        break;
      case BootType::WARM_BOOT:
        stats()->warmBoot();
        break;
      case BootType::UNINITIALIZED:
        CHECK(0);
    }
  }

  if (flags_ & SwitchFlags::PUBLISH_STATS) {
    stats()->multiSwitchStatus(isRunModeMultiSwitch());
  }

  auto bgHeartbeatStatsFunc = [this](int delay, int backLog) {
    stats()->bgHeartbeatDelay(delay);
    stats()->bgEventBacklog(backLog);
  };
  bgThreadHeartbeat_ = std::make_shared<ThreadHeartbeat>(
      &backgroundEventBase_,
      "fbossBgThread",
      FLAGS_thread_heartbeat_ms,
      bgHeartbeatStatsFunc);

  auto updHeartbeatStatsFunc = [this](int delay, int backLog) {
    stats()->updHeartbeatDelay(delay);
    stats()->updEventBacklog(backLog);
  };
  updThreadHeartbeat_ = std::make_shared<ThreadHeartbeat>(
      &updateEventBase_,
      "fbossUpdateThread",
      FLAGS_thread_heartbeat_ms,
      updHeartbeatStatsFunc);

  auto packetTxHeartbeatStatsFunc = [this](int delay, int backLog) {
    stats()->packetTxHeartbeatDelay(delay);
    stats()->packetTxEventBacklog(backLog);
  };
  packetTxThreadHeartbeat_ = std::make_shared<ThreadHeartbeat>(
      &packetTxEventBase_,
      "fbossPktTxThread",
      FLAGS_thread_heartbeat_ms,
      packetTxHeartbeatStatsFunc);

  auto updateLacpThreadHeartbeatStats = [this](int delay, int backLog) {
    stats()->lacpHeartbeatDelay(delay);
    stats()->lacpEventBacklog(backLog);
  };
  lacpThreadHeartbeat_ = std::make_shared<ThreadHeartbeat>(
      &lacpEventBase_,
      *folly::getThreadName(lacpThread_->get_id()),
      FLAGS_thread_heartbeat_ms,
      updateLacpThreadHeartbeatStats);

  neighborCacheThreadHeartbeat_ = std::make_shared<ThreadHeartbeat>(
      &neighborCacheEventBase_,
      *folly::getThreadName(neighborCacheThread_->get_id()),
      FLAGS_thread_heartbeat_ms,
      [this](int delay, int backlog) {
        stats()->neighborCacheHeartbeatDelay(delay);
        stats()->neighborCacheEventBacklog(backlog);
      });

  dsfSubscriberReconnectThreadHeartbeat_ = std::make_shared<ThreadHeartbeat>(
      dsfSubscriber_->getReconnectThreadEvb(),
      "DsfSubscriberReconnectThread",
      FLAGS_dsf_subscriber_reconnect_thread_heartbeat_ms,
      [this](int delay, int backlog) {
        stats()->dsfSubReconnectThreadHeartbeatDelay(delay);
        stats()->dsfSubReconnectThreadEventBacklog(backlog);
      });
  dsfSubscriberStreamThreadHeartbeat_ = std::make_shared<ThreadHeartbeat>(
      dsfSubscriber_->getStreamThreadEvb(),
      "DsfSubscriberStreamThread",
      FLAGS_dsf_subscriber_stream_thread_heartbeat_ms,
      [this](int delay, int backlog) {
        stats()->dsfSubStreamThreadHeartbeatDelay(delay);
        stats()->dsfSubStreamThreadEventBacklog(backlog);
      });

  heartbeatWatchdog_ = std::make_unique<ThreadHeartbeatWatchdog>(
      std::chrono::milliseconds(FLAGS_thread_heartbeat_ms * 10),
      [this]() { stats()->ThreadHeartbeatMissCount(); });
  heartbeatWatchdog_->startMonitoringHeartbeat(bgThreadHeartbeat_);
  heartbeatWatchdog_->startMonitoringHeartbeat(packetTxThreadHeartbeat_);
  heartbeatWatchdog_->startMonitoringHeartbeat(updThreadHeartbeat_);
  heartbeatWatchdog_->startMonitoringHeartbeat(lacpThreadHeartbeat_);
  heartbeatWatchdog_->startMonitoringHeartbeat(neighborCacheThreadHeartbeat_);
  heartbeatWatchdog_->startMonitoringHeartbeat(
      dsfSubscriberReconnectThreadHeartbeat_);
  heartbeatWatchdog_->startMonitoringHeartbeat(
      dsfSubscriberStreamThreadHeartbeat_);
  heartbeatWatchdog_->start();

  setSwitchRunState(SwitchRunState::INITIALIZED);
}

void SwSwitch::stopThreads() {
  // We use runInFbossEventBaseThread() to terminateLoopSoon() rather than
  // calling it directly here.  This ensures that any events already scheduled
  // via runInFbossEventBaseThread() will have a chance to run.
  //
  // Alternatively, it would be nicer to update EventBase so it can notify
  // callbacks when the event loop is being stopped.
  if (backgroundThread_) {
    backgroundEventBase_.runInFbossEventBaseThread(
        [this] { backgroundEventBase_.terminateLoopSoon(); });
  }
  if (updateThread_) {
    updateEventBase_.runInFbossEventBaseThread(
        [this] { updateEventBase_.terminateLoopSoon(); });
  }
  if (packetTxThread_) {
    packetTxEventBase_.runInFbossEventBaseThread(
        [this] { packetTxEventBase_.terminateLoopSoon(); });
  }
  if (packetRxThread_) {
    packetRxRunning_.store(false);
    packetRxEventBase_.runInEventBaseThread(
        [this] { packetRxEventBase_.terminateLoopSoon(); });
  }
  if (pcapDistributionThread_) {
    pcapDistributionEventBase_.runInFbossEventBaseThread(
        [this] { pcapDistributionEventBase_.terminateLoopSoon(); });
  }
  if (lacpThread_) {
    lacpEventBase_.runInFbossEventBaseThread(
        [this] { lacpEventBase_.terminateLoopSoon(); });
  }
  if (neighborCacheThread_) {
    neighborCacheEventBase_.runInFbossEventBaseThread(
        [this] { neighborCacheEventBase_.terminateLoopSoon(); });
  }
  if (backgroundThread_) {
    backgroundThread_->join();
  }
  if (updateThread_) {
    updateThread_->join();
  }
  if (packetTxThread_) {
    packetTxThread_->join();
  }
  if (packetRxThread_) {
    packetRxThread_->join();
  }
  if (pcapDistributionThread_) {
    pcapDistributionThread_->join();
  }
  if (lacpThread_) {
    lacpThread_->join();
  }
  if (neighborCacheThread_) {
    neighborCacheThread_->join();
  }
  // Drain any pending updates by calling handlePendingUpdates. Since
  // we already set state to EXITING, handlePendingUpdates will simply
  // signal the updates and not apply them to HW.
  bool updatesDrained = false;
  do {
    handlePendingUpdates();
    {
      std::unique_lock guard(pendingUpdatesLock_);
      updatesDrained = pendingUpdates_.empty();
    }
  } while (!updatesDrained);
}

void SwSwitch::threadLoop(StringPiece name, folly::EventBase* eventBase) {
  initThread(name);
  eventBase->loopForever();
}

uint32_t SwSwitch::getEthernetHeaderSize() const {
  // VOQ/Fabric switches require that the packets are not VLAN tagged.
  return getSwitchInfoTable().vlansSupported() ? EthHdr::SIZE
                                               : EthHdr::UNTAGGED_PKT_SIZE;
}

std::unique_ptr<TxPacket> SwSwitch::allocatePacket(uint32_t size) const {
  return multiHwSwitchHandler_->allocatePacket(size);
}

std::unique_ptr<TxPacket> SwSwitch::allocateL3TxPacket(uint32_t l3Len) {
  const uint32_t l2Len = getEthernetHeaderSize();
  const uint32_t minLen = 68;
  auto len = std::max(l2Len + l3Len, minLen);
  auto pkt = multiHwSwitchHandler_->allocatePacket(len);
  auto buf = pkt->buf();
  // make sure the whole buffer is available
  buf->clear();
  // reserve for l2 header
  buf->advance(l2Len);
  return pkt;
}

void SwSwitch::sendNetworkControlPacketAsync(
    std::unique_ptr<TxPacket> pkt,
    std::optional<PortDescriptor> portDescriptor) noexcept {
  // TODO(joseph5wu): Control this by distinguishing the highest priority
  // queue from the config.
  static const uint8_t kNCStrictPriorityQueue = 7;

  sendPacketAsync(
      std::move(pkt),
      portDescriptor,
      portDescriptor ? std::make_optional(kNCStrictPriorityQueue)
                     : std::nullopt);
}

void SwSwitch::sendPacketAsync(
    std::unique_ptr<TxPacket> pkt,
    std::optional<PortDescriptor> portDescriptor,
    std::optional<uint8_t> queueId) noexcept {
  if (portDescriptor.has_value()) {
    switch (portDescriptor.value().type()) {
      case PortDescriptor::PortType::PHYSICAL:
        sendPacketOutOfPortAsync(
            std::move(pkt), portDescriptor.value().phyPortID(), queueId);
        break;
      case PortDescriptor::PortType::AGGREGATE:
        sendPacketOutOfPortAsync(
            std::move(pkt), portDescriptor.value().aggPortID(), queueId);
        break;
      case PortDescriptor::PortType::SYSTEM_PORT:
        XLOG(FATAL) << " Packet send over system ports not handled yet";
        break;
    };
  } else {
    CHECK(!queueId.has_value());
    this->sendPacketSwitchedAsync(std::move(pkt));
  }
}

void SwSwitch::sendPacketOutOfPortAsync(
    std::unique_ptr<TxPacket> pkt,
    PortID portID,
    std::optional<uint8_t> queue) noexcept {
  auto state = getState();
  if (!state->getPorts()->getNodeIf(portID)) {
    XLOG(ERR) << "SendPacketOutOfPortAsync: dropping packet to unexpected port "
              << portID;
    stats()->pktDropped();
    return;
  }

  pcapMgr_->packetSent(pkt.get());

  Cursor c(pkt->buf());
  // unused to parse the ethertype correctly
  PktUtil::readMac(&c);
  PktUtil::readMac(&c);
  auto ethertype = c.readBE<uint16_t>();
  if (ethertype == 0x8100) {
    // 802.1Q
    c += 2; // Advance over the VLAN tag.  We ignore it for now
    ethertype = c.readBE<uint16_t>();
  }

  if (!multiHwSwitchHandler_->sendPacketOutOfPortAsync(
          std::move(pkt), portID, queue)) {
    // Just log an error for now.  There's not much the caller can do about
    // send failures--even on successful return from sendPacket*() the
    // send may ultimately fail since it occurs asynchronously in the
    // background.
    XLOG(ERR) << "failed to send packet out port " << portID;
  }
}

void SwSwitch::sendPacketOutOfPortAsync(
    std::unique_ptr<TxPacket> pkt,
    AggregatePortID aggPortID,
    std::optional<uint8_t> queue) noexcept {
  auto aggPort = getState()->getAggregatePorts()->getNodeIf(aggPortID);
  if (!aggPort) {
    XLOG(ERR) << "failed to send packet out aggregate port " << aggPortID
              << ": no aggregate port corresponding to identifier";
    return;
  }

  auto subportAndFwdStates = aggPort->subportAndFwdState();
  if (subportAndFwdStates.begin() == subportAndFwdStates.end()) {
    XLOG(ERR) << "failed to send packet out aggregate port " << aggPortID
              << ": aggregate port has no constituent physical ports";
    return;
  }

  // Ideally, we would select the same (physical) sub-port to send this
  // packet out of as the ASIC. This could be accomplished by mimicking the
  // hardware's logic for doing so, which would for the most part entail
  // computing a hash of the appropriate header fields. Considering the
  // small number of packets that will be forwarded via this path, the added
  // complexity hardly seems worth it.
  // Instead, we simply always choose to send the packet out of the first
  // (physical) sub-port belonging to the aggregate port at hand. This scheme
  // will avoid any issues related to packet reordering. Of course, this
  // will increase the load on the first physical sub-port of each aggregate
  // port, but this imbalance should be negligible.
  PortID subport;
  AggregatePort::Forwarding fwdState;
  for (auto elem : subportAndFwdStates) {
    std::tie(subport, fwdState) = elem;
    if (fwdState == AggregatePort::Forwarding::ENABLED) {
      sendPacketOutOfPortAsync(std::move(pkt), subport, queue);
      return;
    }
  }
  XLOG(DBG2) << "failed to send packet out aggregate port" << aggPortID
             << ": aggregate port has no enabled physical ports";
}

void SwSwitch::sendPacketOutViaThriftStream(
    std::unique_ptr<TxPacket> pkt,
    SwitchID switchId,
    std::optional<PortID> portID,
    std::optional<uint8_t> queue) noexcept {
  multiswitch::TxPacket txPacket;
  if (portID) {
    txPacket.port() = portID.value();
  }
  if (queue) {
    txPacket.queue() = queue.value();
  }
  txPacket.length() = pkt->buf()->computeChainDataLength();
  txPacket.data() = Packet::extractIOBuf(std::move(pkt));
  auto switchIndex =
      getSwitchInfoTable().getSwitchIndexFromSwitchId(SwitchID(switchId));
  try {
    getPacketStreamMap()->getStream(switchId).next(std::move(txPacket));
    stats()->hwAgentTxPktSent(switchIndex);
  } catch (const std::exception&) {
    stats()->pktDropped();
    XLOG(DBG2) << "Error sending packet via thrift stream to switch "
               << switchId;
  }
}

void SwSwitch::sendPacketSwitchedAsync(std::unique_ptr<TxPacket> pkt) noexcept {
  pcapMgr_->packetSent(pkt.get());
  if (!multiHwSwitchHandler_->sendPacketSwitchedAsync(std::move(pkt))) {
    // Just log an error for now.  There's not much the caller can do about
    // send failures--even on successful return from sendPacketSwitchedAsync()
    // the send may ultimately fail since it occurs asynchronously in the
    // background.
    XLOG(ERR) << "failed to send switched packet";
  }
}

std::optional<folly::MacAddress> SwSwitch::getSourceMac(
    const std::shared_ptr<Interface>& intf) const {
  try {
    auto switchIds = getScopeResolver()->scope(intf, getState()).switchIds();
    if (switchIds.empty()) {
      throw FbossError("No switchId scope found for intf: ", intf->getID());
    }
    auto switchId = *switchIds.begin();
    // We always use our CPU's mac-address as source mac-address
    return getHwAsicTable()->getHwAsic(switchId)->getAsicMac();
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Failed to get Mac for intf :" << intf->getID() << " "
              << folly::exceptionStr(ex);
    auto swIds = getHwAsicTable()->getSwitchIDs();
    if (swIds.size()) {
      XLOG(DBG2) << " Falling back to first switchID mac";
      return getHwAsicTable()->getHwAsic(*swIds.begin())->getAsicMac();
    }
  }
  return std::nullopt;
}

void SwSwitch::sendL3Packet(
    std::unique_ptr<TxPacket> pkt,
    InterfaceID ifID) noexcept {
  if (!isFullyInitialized()) {
    XLOG(DBG2) << " Dropping L3 packet since device not yet initialized";
    stats()->pktDropped();
    return;
  }

  // Buffer should not be shared.
  folly::IOBuf* buf = pkt->buf();
  CHECK(!buf->isShared());

  // Add L2 header to L3 packet. Information doesn't need to be complete
  // make sure the packet has enough headroom for L2 header and large enough
  // for the minimum size packet.
  const uint32_t l2Len = getEthernetHeaderSize();
  const uint32_t l3Len = buf->length();
  const uint32_t minLen = 68;
  uint32_t tailRoom = (l2Len + l3Len >= minLen) ? 0 : minLen - l2Len - l3Len;
  if (buf->headroom() < l2Len || buf->tailroom() < tailRoom) {
    XLOG(ERR) << "Packet is not big enough headroom=" << buf->headroom()
              << " required=" << l2Len << ", tailroom=" << buf->tailroom()
              << " required=" << tailRoom;
    stats()->pktError();
    return;
  }

  auto state = getState();

  auto intf = state->getInterfaces()->getNodeIf(ifID);
  if (!intf) {
    XLOG(ERR) << "Interface " << ifID << " doesn't exists in state.";
    stats()->pktDropped();
    return;
  }

  // Extract primary Vlan associated with this interface, if any
  auto vlanID = intf->getVlanIDIf();

  try {
    uint16_t protocol{0};
    folly::IPAddress dstAddr;

    // Parse L3 header to identify IP-Protocol and dstAddr
    folly::io::Cursor cursor(buf);
    uint8_t protoVersion = cursor.read<uint8_t>() >> 4;
    cursor.reset(buf); // Make cursor point to beginning again
    if (protoVersion == 4) {
      protocol = IPv4Handler::ETHERTYPE_IPV4;
      IPv4Hdr ipHdr(cursor);
      dstAddr = ipHdr.dstAddr;
    } else if (protoVersion == 6) {
      protocol = IPv6Handler::ETHERTYPE_IPV6;
      IPv6Hdr ipHdr(cursor);
      dstAddr = ipHdr.dstAddr;
    } else {
      throw FbossError(
          "Wrong version number ",
          static_cast<int>(protoVersion),
          " in the L3 packet to send.");
    }

    // Extend IOBuf to make room for L2 header and satisfy minimum packet size
    buf->prepend(l2Len);
    if (tailRoom) {
      // padding with 0
      memset(buf->writableTail(), 0, tailRoom);
      buf->append(tailRoom);
    }

    auto srcMacOpt = getSourceMac(intf);
    if (!srcMacOpt.has_value()) {
      XLOG(WARNING) << " Failed to get source mac for intf: " << intf->getID();
      return;
    }
    auto srcMac = *srcMacOpt;

    // Derive destination mac address
    folly::MacAddress dstMac{};
    if (dstAddr.isMulticast()) {
      // Multicast Case:
      // Derive destination mac-address based on destination ip-address
      if (dstAddr.isV4()) {
        dstMac = getMulticastMacAddr(dstAddr.asV4());
      } else {
        dstMac = getMulticastMacAddr(dstAddr.asV6());
      }
    } else if (dstAddr.isLinkLocal()) {
      // LinkLocal Case:
      // Resolve neighbor mac address for given destination address. If
      // address doesn't exists in NDP table then request neighbor
      // solicitation for it.
      CHECK(dstAddr.isLinkLocal());
      if (dstAddr.isV4()) {
        // We do not consult ARP table to forward v4 link local addresses.
        // Reason explained below.
        //
        // XXX: In 6Pack/Galaxy ipv4 link-local addresses are routed
        // internally within FCs/LCs so they might not be reachable directly.
        //
        // For now let's make use of L3 table to forward these packets by
        // using dstMac as srcMac
        dstMac = srcMac;
      } else {
        const auto dstAddrV6 = dstAddr.asV6();
        try {
          auto entry = getNeighborEntryForIP<NdpEntry>(
              state, intf, dstAddrV6, FLAGS_intf_nbr_tables);
          if (entry) {
            dstMac = entry->getMac();
          } else {
            // TODO (skhare)
            // The current logic relies on this block throwing an error when
            // neighbor entry is absent so that multicast neighbor
            // solicitation would be issued. There are unit tests (and may be
            // some other assumptions in the code) around it. Thus, retain
            // that behavior for now. However, consider simplifying this to an
            // if-else block instead of try-catch.
            throw FbossError("No neighbor entry for: ", dstAddrV6.str());
          }
        } catch (...) {
          // We don't have dstAddr in our NDP table. Request solicitation for
          // it and let this packet be dropped.
          IPv6Handler::sendMulticastNeighborSolicitation(
              this, dstAddrV6, srcMac, vlanID);
          throw;
        } // try
      }
    } else {
      // Unicast Packet:
      // Ideally we can do routing in SW but it can consume some good ammount
      // of CPU. To avoid this we prefer to perform routing in hardware. Using
      // our CPU MacAddr as DestAddr we will trigger L3 lookup in hardware :)
      dstMac = srcMac;

      // Resolve the l2 address of the next hop if needed. These functions
      // will do the RIB lookup and then probe for any unresolved nexthops
      // of the route.
      if (dstAddr.isV6()) {
        ipv6_->sendMulticastNeighborSolicitations(PortID(0), dstAddr.asV6());
      } else {
        // TODO(skhare) Support optional VLAN arg to resolveMac
        ipv4_->resolveMac(state, PortID(0), dstAddr.asV4());
      }
    }

    // Write L2 header. NOTE that we pass specific VLAN and a dstMac on which
    // packet should be forwarded to.
    folly::io::RWPrivateCursor rwCursor(buf);
    TxPacket::writeEthHeader(&rwCursor, dstMac, srcMac, vlanID, protocol);

    // We can look up the vlan to make sure it exists. However, the vlan can
    // be just deleted after the lookup. So, in order to do it correctly, we
    // will need to lock the state update. Even with that, this still cannot
    // guarantee the vlan we are looking here is the vlan when the packet is
    // originated from the host. Because of that, we are just going to send
    // the packet out to the HW. The HW will drop the packet if the vlan is
    // deleted.
    stats()->pktFromHost(l3Len);
    sendPacketSwitchedAsync(std::move(pkt));
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Failed to send out L3 packet :" << folly::exceptionStr(ex);
  }
}

bool SwSwitch::sendPacketToHost(
    InterfaceID dstIfID,
    std::unique_ptr<RxPacket> pkt) {
  return tunMgr_ && tunMgr_->sendPacketToHost(dstIfID, std::move(pkt));
}

void SwSwitch::applyConfig(const std::string& reason, bool reload) {
  auto applyAgentConfig = [=, this](const AgentConfig* agentConfig) {
    const auto& newConfig = *agentConfig->thrift.sw();
    applyConfigImpl(reason, newConfig);
    agentConfig->dumpConfig(agentDirUtil_->getRunningConfigDumpFile());
  };
  if (reload) {
    auto loadedAgentConfig = loadConfig();
    applyAgentConfig(loadedAgentConfig.get());
    setConfigImpl(std::move(loadedAgentConfig));
    return;
  } else {
    auto agentConfigLocked = agentConfig_.rlock();
    auto agentConfig = agentConfigLocked->get();
    if (agentConfig) {
      applyAgentConfig(agentConfig);
      return;
    }
  }
  applyConfig(reason, true);
}

void SwSwitch::applyConfig(
    const std::string& reason,
    const cfg::SwitchConfig& newConfig) {
  cfg::AgentConfig agentConfigThrift{};
  {
    auto agentConfigLocked = agentConfig_.rlock();
    CHECK(agentConfigLocked->get());
    agentConfigThrift = agentConfigLocked->get()->thrift;
  }
  agentConfigThrift.sw_ref() = newConfig;
  auto newAgentConfig = std::make_unique<AgentConfig>(agentConfigThrift);
  applyConfigImpl(reason, newConfig);
  /* apply and reset software switch config in the already applied agent
   * config
   */
  setConfigImpl(std::move(newAgentConfig));
}

void SwSwitch::applyConfigImpl(
    const std::string& reason,
    const cfg::SwitchConfig& newConfig) {
  // We don't need to hold a lock here. updateStateBlocking() does that for
  // us.
  auto routeUpdater = getRouteUpdater();
  auto oldConfig = getConfig();
  updateStateBlocking(
      reason,
      [&](const shared_ptr<SwitchState>& state) -> shared_ptr<SwitchState> {
        auto originalState = state;
        auto newState = applyThriftConfig(
            originalState,
            &newConfig,
            supportsAddRemovePort_,
            platformMapping_.get(),
            hwAsicTable_.get(),
            &routeUpdater,
            aclNexthopHandler_.get());

        if (newState && !isValidStateUpdate(StateDelta(state, newState))) {
          throw FbossError("Invalid config passed in, skipping");
        }

        if (!newState) {
          // if config is not updated, the new state will return null
          // in such a case return here, to prevent possible crash.
          XLOG(WARNING) << "Applying config did not cause state change";
          return nullptr;
        }
        return newState;
      });
  // Since we're using blocking state update, once we reach here, the new
  // config should be already applied and programmed into hardware.
  updateConfigAppliedInfo();

  /*
   * For RIB always make route programming go through the routeUpdater wrapper
   * abstraction. For rib when routes are applied 2 things happen
   *  - Work is scheduled on rib thread
   *  - Fib update callback is called to program routes. This in turn (for
   *  SwSwitch) *may* schedule updates on SwSwitch::update thread.
   *
   * If we don't use RouteUpdateWrapper flow for config application becomes
   * - schedule a blocking update on SwSwitch::update thread
   *   - schedule work on RIB thread.
   *
   * This is classic lock/blocking work inversion, and creates a deadlock
   * scenario b/w route programming (route add, del or route classID update)
   * and applyConfig. So ensure programming allways goes through the route
   * update wrapper abstraction
   */

  routeUpdater.program();
  runFsdbSyncFunction([&oldConfig, &newConfig](auto& syncer) {
    syncer->cfgUpdated(oldConfig, newConfig);
  });
}

void SwSwitch::updateConfigAppliedInfo() {
  auto lockedConfigAppliedInfo = configAppliedInfo_.wlock();
  auto currentInMs = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch());

  lockedConfigAppliedInfo->lastAppliedInMs() = currentInMs.count();
  // Only need to update `lastColdbootAppliedInMs` once if there's a coldboot
  // since the recent agent restarts
  if (!lockedConfigAppliedInfo->lastColdbootAppliedInMs() &&
      bootType_ == BootType::COLD_BOOT) {
    lockedConfigAppliedInfo->lastColdbootAppliedInMs() = currentInMs.count();
  }

  XLOG(DBG2) << "Finished applied config, lastConfigAppliedInMs="
             << *lockedConfigAppliedInfo->lastAppliedInMs()
             << ", coldboot lastConfigAppliedInMs="
             << (lockedConfigAppliedInfo->lastColdbootAppliedInMs()
                     ? *lockedConfigAppliedInfo->lastColdbootAppliedInMs()
                     : 0);
}

bool SwSwitch::isValidStateUpdate(const StateDelta& delta) const {
  bool isValid = true;

  forEachChanged(
      delta.getAclsDelta(),
      [&](const shared_ptr<AclEntry>& /* oldAcl */,
          const shared_ptr<AclEntry>& newAcl) {
        isValid = isValid && newAcl->hasMatcher();
        return isValid ? LoopAction::CONTINUE : LoopAction::BREAK;
      },
      [&](const shared_ptr<AclEntry>& addAcl) {
        isValid = isValid && addAcl->hasMatcher();
        return isValid ? LoopAction::CONTINUE : LoopAction::BREAK;
      },
      [&](const shared_ptr<AclEntry>& /* delAcl */) {});

  forEachChanged(
      delta.getPortsDelta(),
      [&](const shared_ptr<Port>& /* oldport */,
          const shared_ptr<Port>& newport) {
        isValid = isValid && newport->hasValidPortQueues();
        return isValid ? LoopAction::CONTINUE : LoopAction::BREAK;
      },
      [&](const shared_ptr<Port>& addport) {
        isValid = isValid && addport->hasValidPortQueues();
        return isValid ? LoopAction::CONTINUE : LoopAction::BREAK;
      },
      [&](const shared_ptr<Port>& /* delport */) {});

  // Ensure only one sflow mirror session is configured
  std::set<std::string> ingressMirrors;
  for (auto mniter : std::as_const(*(delta.newState()->getPorts()))) {
    for (auto iter : std::as_const(*mniter.second)) {
      auto port = iter.second;
      if (port && port->getIngressMirror().has_value()) {
        auto ingressMirror = delta.newState()->getMirrors()->getNodeIf(
            port->getIngressMirror().value());
        if (ingressMirror && ingressMirror->type() == Mirror::Type::SFLOW) {
          ingressMirrors.insert(port->getIngressMirror().value());
        }
      }
    }
  }
  if (ingressMirrors.size() > 1) {
    XLOG(ERR) << "Only one sflow mirror can be configured across all ports";
    isValid = false;
  }

  if (isValid) {
    if (isRunModeMonolithic()) {
      isValid = getMonolithicHwSwitchHandler()->isValidStateUpdate(delta);
    } else {
      // TODO - implement state update validation for multiswitch
    }
  }

  return isValid;
}

AdminDistance SwSwitch::clientIdToAdminDistance(int clientId) const {
  auto config = getConfig();
  return getAdminDistanceForClientId(config, clientId);
}

void SwSwitch::clearPortStats(
    const std::unique_ptr<std::vector<int32_t>>& ports) {
  // For multi switch mode, hwswitch clear api gets called instead
  if (isRunModeMonolithic()) {
    getMonolithicHwSwitchHandler()->clearPortStats(ports);
  }
}

std::vector<phy::PrbsLaneStats> SwSwitch::getPortAsicPrbsStats(PortID portId) {
  if (isRunModeMonolithic()) {
    return getMonolithicHwSwitchHandler()->getPortAsicPrbsStats(portId);
  } else {
    throw fboss::FbossError(
        "getPortAsicPrbsStats() not supported for multi switch");
  }
}

void SwSwitch::clearPortAsicPrbsStats(PortID portId) {
  if (isRunModeMonolithic()) {
    return getMonolithicHwSwitchHandler()->clearPortAsicPrbsStats(portId);
  } else {
    throw fboss::FbossError(
        "clearPortAsicPrbsStats() not supported for multi switch");
  }
}

std::vector<prbs::PrbsPolynomial> SwSwitch::getPortPrbsPolynomials(
    PortID portId) {
  auto switchId = getScopeResolver()->scope(portId).switchId();
  auto hwAsic = getHwAsicTable()->getHwAsicIf(switchId);
  return hwAsic->getSupportedPrbsPolynomials();
}

prbs::InterfacePrbsState SwSwitch::getPortPrbsState(PortID portId) {
  if (isRunModeMonolithic()) {
    return getMonolithicHwSwitchHandler()->getPortPrbsState(portId);
  } else {
    throw fboss::FbossError(
        "getPortPrbsState() not supported for multi switch");
  }
}

template <typename AddressT>
std::shared_ptr<Route<AddressT>> SwSwitch::longestMatch(
    std::shared_ptr<SwitchState> state,
    const AddressT& address,
    RouterID vrf) {
  return findLongestMatchRoute(getRib(), vrf, address, state);
}

template std::shared_ptr<Route<folly::IPAddressV4>> SwSwitch::longestMatch(
    std::shared_ptr<SwitchState> state,
    const folly::IPAddressV4& address,
    RouterID vrf);
template std::shared_ptr<Route<folly::IPAddressV6>> SwSwitch::longestMatch(
    std::shared_ptr<SwitchState> state,
    const folly::IPAddressV6& address,
    RouterID vrf);

void SwSwitch::l2LearningUpdateReceived(
    L2Entry l2Entry,
    L2EntryUpdateType l2EntryUpdateType) {
  l2LearnEventObservers_->l2LearnEventReceived(l2Entry, l2EntryUpdateType);
  macTableManager_->handleL2LearningUpdate(l2Entry, l2EntryUpdateType);
}

bool SwSwitch::sendArpRequestHelper(
    std::shared_ptr<Interface> intf,
    std::shared_ptr<SwitchState> state,
    folly::IPAddressV4 source,
    folly::IPAddressV4 target) {
  bool sent = false;
  auto entry = getNeighborEntryForIP<ArpEntry>(
      state, intf, target, FLAGS_intf_nbr_tables);
  if (entry == nullptr) {
    // No entry in ARP table, send ARP request
    ArpHandler::sendArpRequest(
        this, intf->getVlanIDIf(), intf->getMac(), source, target);

    // Notify the updater that we sent an arp request
    sentArpRequest(intf, target);
    sent = true;
  } else {
    XLOG(DBG5) << "not sending arp for " << target.str() << ", "
               << ((entry->isPending()) ? "pending " : "")
               << "entry already exists";
  }

  return sent;
}

bool SwSwitch::sendNdpSolicitationHelper(
    std::shared_ptr<Interface> intf,
    std::shared_ptr<SwitchState> state,
    const folly::IPAddressV6& target) {
  bool sent = false;
  auto entry = getNeighborEntryForIP<NdpEntry>(
      state, intf, target, FLAGS_intf_nbr_tables);
  if (entry == nullptr) {
    // No entry in NDP table, create a neighbor solicitation packet
    IPv6Handler::sendMulticastNeighborSolicitation(
        this, target, intf->getMac(), intf->getVlanIDIf());

    // Notify the updater that we sent a solicitation out
    sentNeighborSolicitation(intf, target);
    sent = true;
  } else {
    XLOG(DBG5) << "not sending neighbor solicitation for " << target.str()
               << ", " << ((entry->isPending()) ? "pending" : "")
               << " entry already exists";
  }

  return sent;
}

VlanID SwSwitch::getVlanIDHelper(std::optional<VlanID> vlanID) const {
  // if vlanID does not have value, it must be VOQ or FABRIC switch
  CHECK(vlanID.has_value() || !getSwitchInfoTable().vlansSupported());

  // TODO(skhare)
  // VOQ/Fabric switches require that the packets are not tagged with any
  // VLAN. We are gradually enhancing wedge_agent to handle tagged as well as
  // untagged packets. During this transition, we will use VlanID 0 to
  // populate SwitchState/Neighbor cache etc. data structures. Once the
  // wedge_agent changes are complete, we will no longer need this function.
  return vlanID.has_value() ? vlanID.value() : VlanID(0);
}

std::optional<VlanID> SwSwitch::getVlanIDForPkt(VlanID vlanID) const {
  // TODO(skhare)
  // VOQ/Fabric switches require that the packets are not tagged with any
  // VLAN. We are gradually enhancing wedge_agent to handle tagged as well as
  // untagged packets. During this transition, we will use VlanID 0 to
  // populate SwitchState/Neighbor cache etc. data structures.
  // However, the packets on wire must not carry VLANs for VOQ/Fabric
  // switches. Once the wedge_agent changes are complete, we will no longer
  // need this function.

  if (!getSwitchInfoTable().vlansSupported()) {
    CHECK_EQ(vlanID, VlanID(0));
    return std::nullopt;
  } else {
    return vlanID;
  }
}

InterfaceID SwSwitch::getInterfaceIDForAggregatePort(
    AggregatePortID aggregatePortID) const {
  auto aggregatePort =
      getState()->getAggregatePorts()->getNodeIf(aggregatePortID);
  CHECK(aggregatePort);
  // On VOQ/Fabric switches, port and interface have 1:1 relation.
  // For non VOQ/Fabric switches, in practice, a port is always part of a
  // single VLAN (and thus single interface).
  CHECK_EQ(aggregatePort->getInterfaceIDs()->size(), 1);
  return InterfaceID(aggregatePort->getInterfaceIDs()->at(0)->cref());
}

void SwSwitch::sentArpRequest(
    const std::shared_ptr<Interface>& intf,
    folly::IPAddressV4 target) {
  if (FLAGS_intf_nbr_tables) {
    getNeighborUpdater()->sentArpRequestForIntf(intf->getID(), target);
  } else {
    getNeighborUpdater()->sentArpRequest(
        getVlanIDHelper(intf->getVlanIDIf()), target);
  }
}

void SwSwitch::sentNeighborSolicitation(
    const std::shared_ptr<Interface>& intf,
    const folly::IPAddressV6& target) {
  if (FLAGS_intf_nbr_tables) {
    getNeighborUpdater()->sentNeighborSolicitationForIntf(
        intf->getID(), target);
  } else {
    getNeighborUpdater()->sentNeighborSolicitation(
        getVlanIDHelper(intf->getVlanIDIf()), target);
  }
}

std::shared_ptr<SwitchState> SwSwitch::stateChanged(
    const StateDelta& delta,
    bool transaction) const {
  return multiHwSwitchHandler_->stateChanged(delta, transaction);
}

std::shared_ptr<SwitchState> SwSwitch::modifyTransceivers(
    const std::shared_ptr<SwitchState>& state,
    const std::unordered_map<TransceiverID, TransceiverInfo>& currentTcvrs,
    const PlatformMapping* platformMapping,
    const SwitchIdScopeResolver* scopeResolver) {
  auto origTcvrs = state->getTransceivers();
  TransceiverMap::NodeContainer newTcvrs;
  bool changed = false;
  for (const auto& tcvrInfo : currentTcvrs) {
    auto origTcvr = origTcvrs->getNodeIf(tcvrInfo.first);
    auto newTcvr = TransceiverSpec::createPresentTransceiver(tcvrInfo.second);
    if (!newTcvr) {
      // If the transceiver used to be present but now was removed
      changed |= (origTcvr != nullptr);
      continue;
    } else {
      if (origTcvr && *origTcvr == *newTcvr) {
        newTcvrs.emplace(origTcvr->getID(), origTcvr);
      } else {
        changed = true;
        newTcvrs.emplace(newTcvr->getID(), newTcvr);
      }
    }
  }

  if (changed) {
    XLOG(DBG2) << "New TransceiverMap has " << newTcvrs.size()
               << " present transceivers, original map has "
               << origTcvrs->size();
    auto newState = state->clone();
    std::unordered_map<PortID, TransceiverID> portIdToTransceiverID;
    const auto& platformPorts = platformMapping->getPlatformPorts();
    for (const auto& [portId, platformPort] : platformPorts) {
      auto transceiverId =
          utility::getTransceiverId(platformPort, platformMapping->getChips());
      if (transceiverId) {
        portIdToTransceiverID.emplace(PortID(portId), *transceiverId);
      }
    }
    auto getPortIdsForTransceiver = [&portIdToTransceiverID](
                                        const TransceiverID& id) {
      std::vector<PortID> portIds;
      for (const auto& [portId, transceiverId] : portIdToTransceiverID) {
        if (id == transceiverId) {
          portIds.emplace_back(portId);
        }
      }
      CHECK_NE(portIds.size(), 0) << "No portId found for transceiver " << id;
      return portIds;
    };
    auto transceiverMap = std::make_shared<TransceiverMap>(std::move(newTcvrs));
    auto multiSwitchTransceiverMap =
        std::make_shared<MultiSwitchTransceiverMap>();
    for (const auto& entry : std::as_const(*transceiverMap)) {
      multiSwitchTransceiverMap->addNode(
          entry.second,
          scopeResolver->scope(
              getPortIdsForTransceiver(TransceiverID(entry.first))));
    }
    newState->resetTransceivers(multiSwitchTransceiverMap);
    return newState;
  } else {
    XLOG(DBG2)
        << "Current transceivers from QsfpCache has the same transceiver size:"
        << origTcvrs->size()
        << ", no need to reset TransceiverMap in current SwitchState";
    return nullptr;
  }
}

folly::MacAddress SwSwitch::getLocalMac(SwitchID switchId) const {
  const auto& switchId2SwitchInfo = switchInfoTable_.getSwitchIdToSwitchInfo();
  auto iter = switchId2SwitchInfo.find(switchId);
  if (iter == switchId2SwitchInfo.end()) {
    throw FbossError("mac can not be found for switch ", switchId);
  }
  if (auto switchMac = iter->second.switchMac()) {
    return folly::MacAddress(*switchMac);
  }
  return getLocalMacAddress();
}

TransceiverIdxThrift SwSwitch::getTransceiverIdxThrift(PortID portID) const {
  auto port = getState()->getPorts()->getNode(portID);

  TransceiverIdxThrift idx{};
  idx.transceiverId() = platformMapping_->getTransceiverIdFromSwPort(portID);
  PlatformPortProfileConfigMatcher matcher(port->getProfileID(), portID);
  std::vector<int32_t> lanes;
  auto hostLanes = platformMapping_->getTransceiverHostLanes(matcher);
  std::copy(hostLanes.begin(), hostLanes.end(), std::back_inserter(lanes));
  idx.channels() = std::move(lanes);
  return idx;
}

std::optional<uint32_t> SwSwitch::getHwLogicalPortId(PortID portID) const {
  auto portStats = getHwPortStats({portID});
  if (!portStats.empty() &&
      portStats.begin()->second.logicalPortId().has_value()) {
    return portStats.begin()->second.logicalPortId().value();
  }
  return std::nullopt;
}

const AgentDirectoryUtil* SwSwitch::getDirUtil() const {
  return agentDirUtil_;
}

void SwSwitch::storeWarmBootState(const state::WarmbootState& state) {
  swSwitchWarmbootHelper_->storeWarmBootState(state);
}

void SwSwitch::updateDsfSubscriberState(
    const std::string& remoteEndpoint,
    fsdb::FsdbSubscriptionState oldState,
    fsdb::FsdbSubscriptionState newState) {
  runFsdbSyncFunction([remoteEndpoint = remoteEndpoint, oldState, newState](
                          auto& syncer) mutable {
    syncer->updateDsfSubscriberState(remoteEndpoint, oldState, newState);
  });
}

std::string SwSwitch::getConfigStr() const {
  return apache::thrift::SimpleJSONSerializer::serialize<std::string>(
      getConfig());
}

cfg::SwitchConfig SwSwitch::getConfig() const {
  const auto& agentConfigLocked = agentConfig_.rlock();
  if (!agentConfigLocked->get()) {
    return cfg::SwitchConfig();
  }
  return agentConfigLocked->get()->thrift.sw().value();
}

cfg::AgentConfig SwSwitch::getAgentConfig() const {
  const auto& agentConfigLocked = agentConfig_.rlock();
  if (!agentConfigLocked->get()) {
    return cfg::AgentConfig();
  }
  return agentConfigLocked->get()->thrift;
}

std::unique_ptr<AgentConfig> SwSwitch::loadConfig() {
  try {
    return AgentConfig::fromDefaultFile();
  } catch (const std::exception&) {
    XLOG(ERR) << "Couldn't load agent config from default file";
    throw;
  }
}

void SwSwitch::setConfig(std::unique_ptr<AgentConfig> config) {
  // used only for tests
  setConfigImpl(std::move(config));
}

void SwSwitch::setConfigImpl(std::unique_ptr<AgentConfig> config) {
  auto agentConfigLocked = agentConfig_.wlock();
  *agentConfigLocked = std::move(config);
}

bool SwSwitch::needL2EntryForNeighbor() const {
  if (!isFullyConfigured()) {
    return getHwSwitchHandler()->needL2EntryForNeighbor(nullptr);
  }
  const auto& config = getConfig();
  return getHwSwitchHandler()->needL2EntryForNeighbor(&config);
}

void SwSwitch::updateHwSwitchStats(
    uint16_t switchIndex,
    multiswitch::HwSwitchStats hwStats) {
  (*hwSwitchStats_.wlock())[switchIndex] = std::move(hwStats);
}

multiswitch::HwSwitchStats SwSwitch::getHwSwitchStatsExpensive(
    uint16_t switchIndex) const {
  auto lockedStats = hwSwitchStats_.rlock();
  if (lockedStats->find(switchIndex) == lockedStats->end()) {
    throw FbossError("No stats for switch index ", switchIndex);
  }
  return lockedStats->at(switchIndex);
}

std::map<uint16_t, multiswitch::HwSwitchStats>
SwSwitch::getHwSwitchStatsExpensive() const {
  return *hwSwitchStats_.rlock();
}

FabricReachabilityStats SwSwitch::getFabricReachabilityStats() {
  auto lockedStats = hwSwitchStats_.rlock();
  std::map<uint16_t, FabricReachabilityStats> fabricReachStats;
  FabricReachabilityStats reachStats;
  for (const auto& [_, stats] : *lockedStats) {
    *reachStats.missingCount() +=
        *stats.fabricReachabilityStats()->missingCount();
    *reachStats.mismatchCount() +=
        *stats.fabricReachabilityStats()->mismatchCount();
    *reachStats.virtualDevicesWithAsymmetricConnectivity() +=
        *stats.fabricReachabilityStats()
             ->virtualDevicesWithAsymmetricConnectivity();
  }
  return reachStats;
}

void SwSwitch::setPortsDownForSwitch(SwitchID switchId) {
  for (const auto& [matcher, portMap] :
       std::as_const(*getState()->getPorts())) {
    // walk through all ports on the switch and set them down
    if (HwSwitchMatcher(matcher).has(switchId)) {
      for (const auto& port : std::as_const(*portMap)) {
        if (port.second->isUp()) {
          linkStateChanged(
              port.second->getID(), false, port.second->getPortType());
        }
      }
    }
  }
}

std::map<PortID, HwPortStats> SwSwitch::getHwPortStats(
    std::vector<PortID> ports) const {
  std::map<PortID, HwPortStats> hwPortsStats;
  for (const auto& portId : ports) {
    auto switchIds = getScopeResolver()->scope(portId).switchIds();
    CHECK_EQ(switchIds.size(), 1);
    auto switchIndex =
        getSwitchInfoTable().getSwitchIndexFromSwitchId(*switchIds.cbegin());
    auto hwswitchStatsMap = hwSwitchStats_.rlock();
    auto hwswitchStats = hwswitchStatsMap->find(switchIndex);
    if (hwswitchStats != hwswitchStatsMap->end()) {
      auto portName = getState()->getPorts()->getNodeIf(portId)->getName();
      auto statsMap = hwswitchStats->second.hwPortStats();
      auto entry = statsMap->find(portName);
      if (entry != statsMap->end()) {
        hwPortsStats.insert(
            {portId, hwswitchStats->second.hwPortStats()->at(portName)});
      }
    }
  }
  return hwPortsStats;
}

std::map<SystemPortID, HwSysPortStats> SwSwitch::getHwSysPortStats(
    const std::vector<SystemPortID>& ports) const {
  std::map<SystemPortID, HwSysPortStats> hwPortsStats;
  for (const auto& portId : ports) {
    auto switchIds = getScopeResolver()->scope(portId).switchIds();
    CHECK_EQ(switchIds.size(), 1);
    auto switchIndex =
        getSwitchInfoTable().getSwitchIndexFromSwitchId(*switchIds.cbegin());
    auto hwswitchStatsMap = hwSwitchStats_.rlock();
    auto hwswitchStats = hwswitchStatsMap->find(switchIndex);
    if (hwswitchStats != hwswitchStatsMap->end()) {
      auto portName =
          getState()->getSystemPorts()->getNodeIf(portId)->getName();
      auto statsMap = hwswitchStats->second.sysPortStats();
      auto entry = statsMap->find(portName);
      if (entry != statsMap->end()) {
        hwPortsStats.insert(
            {portId, hwswitchStats->second.sysPortStats()->at(portName)});
      }
    }
  }
  return hwPortsStats;
}

void SwSwitch::stopHwSwitchHandler() {
  if (isRunModeMonolithic()) {
    getMonolithicHwSwitchHandler()->gracefulExit();
  }
  multiHwSwitchHandler_->stop();
}

void SwSwitch::updateAddrToLocalIntf(const StateDelta& delta) {
  DeltaFunctions::forEachChanged(
      delta.getIntfsDelta(),
      [&](const auto& oldNode, const auto& newNode) {
        const auto oldRouterId = oldNode->getRouterID();
        for (const auto& [addr, _] : std::as_const(*oldNode->getAddresses())) {
          addrToLocalIntf_.erase(
              std::make_pair(oldRouterId, folly::IPAddress(addr)));
        }
        const auto newRouterId = newNode->getRouterID();
        for (const auto& [addr, _] : std::as_const(*newNode->getAddresses())) {
          addrToLocalIntf_.insert(
              std::make_pair(newRouterId, folly::IPAddress(addr)),
              newNode->getID());
        }
      },
      [&](const auto& added) {
        const auto routerId = added->getRouterID();
        for (const auto& [addr, _] : std::as_const(*added->getAddresses())) {
          addrToLocalIntf_.insert(
              std::make_pair(routerId, folly::IPAddress(addr)), added->getID());
        }
      },
      [&](const auto& removed) {
        const auto routerId = removed->getRouterID();
        for (const auto& [addr, _] : std::as_const(*removed->getAddresses())) {
          addrToLocalIntf_.erase(
              std::make_pair(routerId, folly::IPAddress(addr)));
        }
      });
}

void SwSwitch::rxPacketReceived(std::unique_ptr<SwRxPacket> pkt) {
  folly::coro::blockingWait(
      rxPacketHandlerQueues_.enqueue(std::move(pkt), stats()));
  rxPktThreadCV_.notify_one();
}

} // namespace facebook::fboss
