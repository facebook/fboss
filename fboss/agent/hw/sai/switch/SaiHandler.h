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

#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/hw/sai/diag/DiagShell.h"
#include "fboss/agent/hw/sai/switch/gen-cpp2/SaiCtrl.h"

namespace facebook::fboss {

class DiagCmdServer;
class SaiSwitch;
class StreamingDiagShellServer;

class SaiHandler : public apache::thrift::ServiceHandler<SaiCtrl> {
 public:
  explicit SaiHandler(SaiSwitch* hw);
  ~SaiHandler() override;

  void getCurrentHwStateJSON(
      std::string& ret,
      std::unique_ptr<std::string> path) override;

  /*
   * Get live serialized switch state for provided paths
   */
  void getCurrentHwStateJSONForPaths(
      std::map<std::string, std::string>& pathToState,
      std::unique_ptr<std::vector<std::string>> paths) override;

  apache::thrift::ResponseAndServerStream<std::string, std::string>
  startDiagShell() override;
  void produceDiagShellInput(
      std::unique_ptr<std::string> input,
      std::unique_ptr<ClientInformation> client) override;

  void diagCmd(
      fbstring& result,
      std::unique_ptr<fbstring> cmd,
      std::unique_ptr<ClientInformation> client,
      int16_t serverTimeoutMsecs = 0,
      bool bypassFilter = false) override;

  SwitchRunState getHwSwitchRunState() override;

  void getHwFabricReachability(
      std::map<::std::int64_t, ::facebook::fboss::FabricEndpoint>& reachability)
      override;
  void getHwFabricConnectivity(
      std::map<::std::string, ::facebook::fboss::FabricEndpoint>& connectivity)
      override;
  void getHwSwitchReachability(
      std::map<::std::string, std::vector<::std::string>>& reachability,
      std::unique_ptr<::std::vector<::std::string>> switchNames) override;
  void clearHwPortStats(std::unique_ptr<std::vector<int32_t>> ports) override;
  void clearAllHwPortStats() override;
  void clearInterfacePhyCounters(
      std::unique_ptr<std::vector<int32_t>> ports) override;
  void getHwL2Table(std::vector<L2EntryThrift>& l2Table) override;
  void getVirtualDeviceToConnectionGroups(
      std::map<
          int64_t,
          std::map<int64_t, std::vector<facebook::fboss::RemoteEndpoint>>>&
          virtualDevice2ConnectionGroups) override;

  void listHwObjects(
      std::string& out,
      std::unique_ptr<std::vector<HwObjectType>> hwObjects,
      bool cached) override;

  BootType getBootType() override;

  void getInterfacePrbsState(
      prbs::InterfacePrbsState& prbsState,
      std::unique_ptr<std::string> interface,
      phy::PortComponent component) override;

  void getAllInterfacePrbsStates(
      std::map<std::string, prbs::InterfacePrbsState>& prbsStates,
      phy::PortComponent component) override;

  void getInterfacePrbsStats(
      phy::PrbsStats& prbsStats,
      std::unique_ptr<std::string> interface,
      phy::PortComponent component) override;

  void getAllInterfacePrbsStats(
      std::map<std::string, phy::PrbsStats>& prbsStats,
      phy::PortComponent component) override;

  void clearInterfacePrbsStats(
      std::unique_ptr<std::string> interface,
      phy::PortComponent component) override;

  void bulkClearInterfacePrbsStats(
      std::unique_ptr<std::vector<std::string>> interfaces,
      phy::PortComponent component) override;

  void reconstructSwitchState(state::SwitchState& /* state */) override {
    throw FbossError(
        "reconstructSwitchState Not implemented in MultiSwitchHwSwitchHandler");
  }

  void getAllHwFirmwareInfo(
      std::vector<FirmwareInfo>& firmwareInfoList) override;

  void getHwDebugDump(std::string& out) override;

  void getPortPrbsState(prbs::InterfacePrbsState& prbsState, int32_t portId)
      override;
  void getPortAsicPrbsStats(
      std::vector<phy::PrbsLaneStats>& prbsStats,
      int32_t portId) override;
  void clearPortAsicPrbsStats(int32_t portId) override;
  void getPortPrbsPolynomials(
      std::vector<prbs::PrbsPolynomial>& prbsPolynomials,
      int32_t portId) override;

  void getProgrammedState(state::SwitchState& state) override;

 private:
  SaiSwitch* hw_;
  StreamingDiagShellServer diagShell_;
  DiagCmdServer diagCmdServer_;
  std::mutex diagCmdLock_;
};

} // namespace facebook::fboss
