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
#include <memory>
#include <string>
#include <vector>

#include "common/fb303/cpp/FacebookBase2.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/if/gen-cpp2/NeighborListenerClient.h"
#include "fboss/agent/types.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/lib/phy/gen-cpp2/prbs_types.h"

#include <folly/String.h>
#include <folly/Synchronized.h>
#include <thrift/lib/cpp/server/TServerEventHandler.h>
#include <thrift/lib/cpp2/async/DuplexChannel.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>

DECLARE_bool(disable_duplex);

namespace facebook::fboss {

class AggregatePort;
class Port;
class SwSwitch;
class Vlan;
class SwitchState;
class AclEntry;
struct LinkNeighbor;

class ThriftHandler : virtual public FbossCtrlSvIf,
                      public fb303::FacebookBase2,
                      public apache::thrift::server::TServerEventHandler {
 public:
  template <typename T>
  using ThriftCallback = std::unique_ptr<apache::thrift::HandlerCallback<T>>;
  using TConnectionContext = apache::thrift::server::TConnectionContext;

  typedef network::thrift::Address Address;
  typedef network::thrift::BinaryAddress BinaryAddress;
  typedef folly::EventBase EventBase;
  typedef std::vector<Address> Addresses;
  typedef std::vector<BinaryAddress> BinaryAddresses;

  explicit ThriftHandler(SwSwitch* sw);

  fb303::cpp2::fb_status getStatus() override;

  void async_tm_getStatus(ThriftCallback<fb303::cpp2::fb_status> cb) override;

  void async_eb_registerForNeighborChanged(
      ThriftCallback<void> callback) override;

  void flushCountersNow() override;

  void addUnicastRoute(int16_t client, std::unique_ptr<UnicastRoute> route)
      override;
  void deleteUnicastRoute(int16_t client, std::unique_ptr<IpPrefix> prefix)
      override;
  void addUnicastRoutes(
      int16_t client,
      std::unique_ptr<std::vector<UnicastRoute>> routes) override;
  void deleteUnicastRoutes(
      int16_t client,
      std::unique_ptr<std::vector<IpPrefix>> prefixes) override;
  void syncFib(
      int16_t client,
      std::unique_ptr<std::vector<UnicastRoute>> routes) override;

  void addUnicastRouteInVrf(
      int16_t client,
      std::unique_ptr<UnicastRoute> route,
      int32_t vrf) override;
  void deleteUnicastRouteInVrf(
      int16_t client,
      std::unique_ptr<IpPrefix> prefix,
      int32_t vrf) override;
  void addUnicastRoutesInVrf(
      int16_t client,
      std::unique_ptr<std::vector<UnicastRoute>> routes,
      int32_t vrf) override;
  void deleteUnicastRoutesInVrf(
      int16_t client,
      std::unique_ptr<std::vector<IpPrefix>> prefixes,
      int32_t vrf) override;
  void syncFibInVrf(
      int16_t client,
      std::unique_ptr<std::vector<UnicastRoute>> routes,
      int32_t vrf) override;

  /* MPLS routes */
  void addMplsRoutes(
      int16_t clientId,
      std::unique_ptr<std::vector<MplsRoute>> mplsRoutes) override;

  void deleteMplsRoutes(
      int16_t client,
      std::unique_ptr<std::vector<int32_t>> topLabels) override;
  void syncMplsFib(
      int16_t client,
      std::unique_ptr<std::vector<MplsRoute>> mplsRoutes) override;
  void getMplsRouteTableByClient(
      std::vector<MplsRoute>& mplsRoutes,
      int16_t clientId) override;

  void getAllMplsRouteDetails(
      std::vector<MplsRouteDetails>& mplsRouteDetails) override;

  void getMplsRouteDetails(
      MplsRouteDetails& mplsRouteDetail,
      MplsLabel topLabel) override;

  void addTeFlows(
      std::unique_ptr<std::vector<FlowEntry>> teFlowEntries) override;
  void deleteTeFlows(std::unique_ptr<std::vector<TeFlow>> teFlows) override;
  void syncTeFlows(
      std::unique_ptr<std::vector<FlowEntry>> teFlowEntries) override;

  SwSwitch* getSw() const {
    return sw_;
  }

  void sendPkt(int32_t port, int32_t vlan, std::unique_ptr<fbstring> data)
      override;
  void sendPktHex(int32_t port, int32_t vlan, std::unique_ptr<fbstring> hex)
      override;

  void txPkt(int32_t port, std::unique_ptr<fbstring> data) override;
  void txPktL2(std::unique_ptr<fbstring> data) override;
  void txPktL3(std::unique_ptr<fbstring> payload) override;

  int32_t flushNeighborEntry(std::unique_ptr<BinaryAddress> ip, int32_t vlan)
      override;

  void getVlanAddresses(Addresses& addrs, int32_t vlan) override;
  void getVlanAddressesByName(
      Addresses& addrs,
      const std::unique_ptr<std::string> vlan) override;
  void getVlanBinaryAddresses(BinaryAddresses& addrs, int32_t vlan) override;
  void getVlanBinaryAddressesByName(
      BinaryAddresses& addrs,
      const std::unique_ptr<std::string> vlan) override;
  /* Returns the Ip Route for the address */
  void getIpRoute(
      UnicastRoute& route,
      std::unique_ptr<Address> addr,
      int32_t vrfId) override;
  void getIpRouteDetails(
      RouteDetails& route,
      std::unique_ptr<Address> addr,
      int32_t vrfId) override;
  void getAllInterfaces(
      std::map<int32_t, InterfaceDetail>& interfaces) override;
  void getInterfaceList(std::vector<std::string>& interfaceList) override;

  void getRouteTable(std::vector<UnicastRoute>& routeTable) override;
  void getRouteTableByClient(
      std::vector<UnicastRoute>& routeTable,
      int16_t clientId) override;
  void getRouteTableDetails(std::vector<RouteDetails>& routeTable) override;

  void getPortStatus(
      std::map<int32_t, PortStatus>& status,
      std::unique_ptr<std::vector<int32_t>> ports) override;
  void setPortState(int32_t portId, bool enable) override;
  void setPortDrainState(int32_t portId, bool drain) override;
  void setPortLoopbackMode(int32_t portId, PortLoopbackMode mode) override;
  void getAllPortLoopbackMode(
      std::map<int32_t, PortLoopbackMode>& port2LbMode) override;

  void programInternalPhyPorts(
      std::map<int32_t, cfg::PortProfileID>& programmedPorts,
      std::unique_ptr<TransceiverInfo> transceiver,
      bool force) override;

  void clearPortPrbsStats(int32_t portId, phy::PortComponent component)
      override;
  void getPortPrbsStats(
      phy::PrbsStats& prbsStats,
      int32_t portId,
      phy::PortComponent component) override;
  void setPortPrbs(
      int32_t portId,
      phy::PortComponent component,
      bool enable,
      int32_t polynominal) override;
  void getSupportedPrbsPolynomials(
      std::vector<prbs::PrbsPolynomial>& prbsCapabilities,
      std::unique_ptr<std::string> portName,
      phy::PortComponent component) override;
  void getInterfacePrbsState(
      prbs::InterfacePrbsState& prbsState,
      std::unique_ptr<std::string> portName,
      phy::PortComponent component) override;
  void setInterfacePrbs(
      std::unique_ptr<std::string> portName,
      phy::PortComponent component,
      std::unique_ptr<prbs::InterfacePrbsState> state) override;
  void getInterfacePrbsStats(
      phy::PrbsStats& response,
      std::unique_ptr<std::string> portName,
      phy::PortComponent component) override;
  void clearInterfacePrbsStats(
      std::unique_ptr<std::string> portName,
      phy::PortComponent component) override;
  void getInterfaceDetail(
      InterfaceDetail& interfaceDetails,
      int32_t interfaceId) override;
  void getPortInfo(PortInfoThrift& portInfo, int32_t portId) override;
  void getAllPortInfo(std::map<int32_t, PortInfoThrift>& portInfo) override;
  void clearPortStats(std::unique_ptr<std::vector<int32_t>> ports) override;
  void clearAllPortStats() override;
  void getPortStats(PortInfoThrift& portInfo, int32_t portId) override;
  void getAllPortStats(std::map<int32_t, PortInfoThrift>& portInfo) override;
  void getRunningConfig(std::string& configStr) override;
  void getArpTable(std::vector<ArpEntryThrift>& arpTable) override;
  void getL2Table(std::vector<L2EntryThrift>& l2Table) override;
  void getAclTable(std::vector<AclEntryThrift>& AclTable) override;
  void getAggregatePort(
      AggregatePortThrift& aggregatePortThrift,
      int32_t aggregatePortIDThrift) override;
  void getAggregatePortTable(
      std::vector<AggregatePortThrift>& aggregatePortsThrift) override;
  void getNdpTable(std::vector<NdpEntryThrift>& arpTable) override;
  void getLacpPartnerPair(LacpPartnerPair& lacpPartnerPair, int32_t portID)
      override;
  void getAllLacpPartnerPairs(
      std::vector<LacpPartnerPair>& lacpPartnerPairs) override;

  /* returns the product information */
  void getProductInfo(ProductInfo& productInfo) override;

  BootType getBootType() override;

  void getLldpNeighbors(std::vector<LinkNeighborThrift>& results) override;

  void startPktCapture(std::unique_ptr<CaptureInfo> info) override;
  void stopPktCapture(std::unique_ptr<std::string> name) override;
  void stopAllPktCaptures() override;

  void startLoggingRouteUpdates(
      std::unique_ptr<RouteUpdateLoggingInfo> info) override;
  void stopLoggingRouteUpdates(
      std::unique_ptr<IpPrefix> prefix,
      std::unique_ptr<std::string> identifier) override;
  void stopLoggingAnyRouteUpdates(
      std::unique_ptr<std::string> identifier) override;
  void getRouteUpdateLoggingTrackedPrefixes(
      std::vector<RouteUpdateLoggingInfo>& infos) override;

  void startLoggingMplsRouteUpdates(
      std::unique_ptr<MplsRouteUpdateLoggingInfo> info) override;
  void stopLoggingMplsRouteUpdates(
      std::unique_ptr<MplsRouteUpdateLoggingInfo> info) override;
  void stopLoggingAnyMplsRouteUpdates(
      std::unique_ptr<std::string> identifier) override;
  void getMplsRouteUpdateLoggingTrackedLabels(
      std::vector<MplsRouteUpdateLoggingInfo>& infos) override;

  void getRouteCounterBytes(
      std::map<std::string, std::int64_t>& routeCounters,
      std::unique_ptr<std::vector<std::string>> counters) override;
  void getAllRouteCounterBytes(
      std::map<std::string, std::int64_t>& routeCounters) override;

  void getTeFlowTableDetails(std::vector<TeFlowDetails>& flowTable) override;
  void getFabricReachability(
      std::map<std::string, FabricEndpoint>& reachability) override;
  void getSwitchReachability(
      std::map<std::string, std::vector<std::string>>& reachabilityMatrix,
      std::unique_ptr<std::vector<std::string>> switchNames) override;
  void getDsfNodes(std::map<int64_t, cfg::DsfNode>& dsfNodes) override;
  void getSystemPorts(std::map<int64_t, SystemPortThrift>& sysPorts) override;
  void getSysPortStats(
      std::map<std::string, HwSysPortStats>& hwSysPortStats) override;
  void getCpuPortStats(CpuPortStats& hwCpuPortStats) override;
  void getHwPortStats(std::map<std::string, HwPortStats>& hwPortStats) override;
  /*
   * Event handler for when a connection is destroyed.  When there is an ongoing
   * duplex connection, there may be other threads that depend on the connection
   * state.
   *
   * @param[in]   ctx   A pointer to the connection context that is being
   *                    destroyed.
   */
  void connectionDestroyed(
      apache::thrift::server::TConnectionContext* ctx) override;

  /*
   * Thrift handler for keepalive messages.  It's a no-op, but prevents the
   * server from hitting an idle timeout while it's still publishing samples.
   *
   * @param[in]    callback    The callback for after we finish processing the
   *                           request.
   */
  void async_tm_keepalive(ThriftCallback<void> callback) override {
    callback->done();
  }

  /*
   * Indicate a change in the parent ThriftServer's idle timeout.  NOT a thrift
   * call.  This must be called before any client calls the getIdleTimeout()
   * Thrift function or it will throw an FbossError.  It is not always set
   * because sometimes we want to create a ThriftHandler without a ThriftServer
   * (e.g., during unit tests).
   *
   * @param[in]   timeout      The idle timeout in seconds.
   */
  void setIdleTimeout(const int32_t timeout) {
    thriftIdleTimeout_ = timeout;
  }

  /*
   * Thrift call to get the server's idle timeout.  Used by duplex clients to
   * configure keepalive intervals. If the timeout is unset of <0 (invalid) this
   * call throws an FbossError.
   *
   * @return    The idle timeout in seconds.
   */
  int32_t getIdleTimeout() override;

  /**
   * Thrift call to force reload the config from config file flag. This is
   * useful if we change the config file while the agent is running, and wish
   * to update its config to most recent version.
   */
  void reloadConfig() override;

  /*
   * Get last time(ms since epoch) of the config is applied.
   * NOTE: If no config has ever been applied, the default timestamp is 0.
   * TODO(joseph5wu) Will deprecate such api and use getConfigAppliedInfo()
   * instead
   */
  int64_t getLastConfigAppliedInMs() override;

  /*
   * Get config applied information, which includes last config applied time(ms)
   * and last coldboot config applied time(ms).
   */
  void getConfigAppliedInfo(ConfigAppliedInfo& configAppliedInfo) override;

  /**
   * Serialize live running switch state at the path pointer by thrift path
   */
  void getCurrentStateJSON(std::string& ret, std::unique_ptr<std::string> path)
      override;

  /**
   * Patch live running switch state at path pointed by jsonPointer using the
   * JSON merge patch supplied in jsonPatch
   */
  void patchCurrentStateJSON(
      std::unique_ptr<std::string> jsonPointer,
      std::unique_ptr<std::string> jsonPatch) override;

  SwitchRunState getSwitchRunState() override;

  void setSSLPolicy(apache::thrift::SSLPolicy sslPolicy) {
    sslPolicy_ = sslPolicy;
  }

  SSLType getSSLPolicy() override;

  void setExternalLedState(int32_t portNum, PortLedExternalState ledState)
      override;

  void getHwDebugDump(std::string& out) override;
  void listHwObjects(
      std::string& out,
      std::unique_ptr<std::vector<HwObjectType>> hwObjects,
      bool cached) override;

  void getPlatformMapping(cfg::PlatformMapping& ret) override;

  void getBlockedNeighbors(
      std::vector<cfg::Neighbor>& blockedNeighbors) override;
  void setNeighborsToBlock(
      std::unique_ptr<std::vector<cfg::Neighbor>> neighborsToBlock) override;

  void getMacAddrsToBlock(
      std::vector<cfg::MacAndVlan>& blockedMacAddrs) override;
  void setMacAddrsToBlock(
      std::unique_ptr<std::vector<cfg::MacAndVlan>> macAddrsToBlock) override;

  void publishLinkSnapshots(
      std::unique_ptr<std::vector<std::string>> portNames) override;

  void getInterfacePhyInfo(
      std::map<std::string, phy::PhyInfo>& phyInfos,
      std::unique_ptr<std::vector<std::string>> portNames) override;
  bool isSwitchDrained() override;

 protected:
  void addMplsRoutesImpl(
      std::shared_ptr<SwitchState>* state,
      ClientID clientId,
      const std::vector<MplsRoute>& mplsRoutes) const;
  void addMplsRibRoutes(
      int16_t clientId,
      std::unique_ptr<std::vector<MplsRoute>> mplsRoutes,
      bool sync) const;
  void deleteMplsRibRoutes(
      int16_t clientId,
      std::unique_ptr<std::vector<MplsLabel>> mplsRoutes) const;
  void getPortStatusImpl(
      std::map<int32_t, PortStatus>& statusMap,
      const std::unique_ptr<std::vector<int32_t>>& ports) const;

  void ensureConfigured(folly::StringPiece function) const;
  void ensureConfigured() const {
    // This version of ensureConfigured() won't log
    ensureConfigured(folly::StringPiece(nullptr, nullptr));
  }

 private:
  void ensureNPU(folly::StringPiece function) const;
  void ensureNotFabric(folly::StringPiece function) const;
  void ensureVoqOrFabric(folly::StringPiece function) const;
  struct ThreadLocalListener {
    EventBase* eventBase;
    std::unordered_map<
        const apache::thrift::server::TConnectionContext*,
        std::shared_ptr<NeighborListenerClientAsyncClient>>
        clients;

    explicit ThreadLocalListener(EventBase* eb) : eventBase(eb){};
  };
  folly::ThreadLocalPtr<ThreadLocalListener, int> listeners_;
  void invokeNeighborListeners(
      ThreadLocalListener* info,
      std::vector<std::string> added,
      std::vector<std::string> deleted);
  void updateUnicastRoutesImpl(
      int32_t vrf,
      int16_t client,
      const std::unique_ptr<std::vector<UnicastRoute>>& routes,
      const std::string& updType,
      bool sync);

  void fillPortStats(PortInfoThrift& portInfo, int numPortQs = 0);

  Vlan* getVlan(int32_t vlanId);
  Vlan* getVlan(const std::string& vlanName);
  template <typename ADDR_TYPE, typename ADDR_CONVERTER>
  void getVlanAddresses(
      const Vlan* vlan,
      std::vector<ADDR_TYPE>& addrs,
      ADDR_CONVERTER& converter);
  // Forbidden copy constructor and assignment operator
  ThriftHandler(ThriftHandler const&) = delete;
  ThriftHandler& operator=(ThriftHandler const&) = delete;

  template <typename Result>
  void fail(const ThriftCallback<Result>& callback, const std::exception& ex) {
    FbossError error(folly::exceptionStr(ex));
    callback->exception(error);
  }
  template <typename AddressT, typename NeighborThriftT>
  void addRemoteNeighbors(
      const std::shared_ptr<SwitchState> state,
      std::vector<NeighborThriftT>& nbrs) const;

  /*
   * A pointer to the SwSwitch.  We don't own this.
   * It's the main program's responsibility to ensure that the SwSwitch exists
   * for the lifetime of the ThriftHandler.
   */
  SwSwitch* sw_;

  int thriftIdleTimeout_;
  std::vector<const TConnectionContext*> brokenClients_;

  apache::thrift::SSLPolicy sslPolicy_;

  std::unordered_set<uint16_t> syncedFibClients;
};

} // namespace facebook::fboss
