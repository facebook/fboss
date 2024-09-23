// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/FbossInit.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/gen-cpp2/production_features_types.h"
#include "fboss/agent/test/utils/AgentHwTestConstants.h"

#include <gflags/gflags.h>
#include <gtest/gtest.h>

DECLARE_int32(update_watermark_stats_interval_s);
DECLARE_int32(update_voq_stats_interval_s);
DECLARE_int32(update_cable_length_stats_s);
DECLARE_bool(publish_state_to_fsdb);
DECLARE_bool(publish_stats_to_fsdb);
DECLARE_bool(intf_nbr_tables);
DECLARE_bool(classid_for_unresolved_routes);
DECLARE_bool(disable_neighbor_updates);
DECLARE_bool(disable_icmp_error_response);
DECLARE_bool(enable_snapshot_debugs);
DECLARE_bool(disable_looped_fabric_ports);
DECLARE_bool(dsf_subscribe);

namespace facebook::fboss {

/*
 * AgentHwTest is the base class for agent tests running in mono or split
 * mode. At early phases, the tests will be running in both mono and split mode
 * to assist migration. This should eventually be named as AgentTest as all
 * Agent/Hw tests are migrated to this framework.
 */

class AgentHwTest : public ::testing::Test {
 public:
  AgentHwTest() = default;
  ~AgentHwTest() override = default;
  void SetUp() override;
  void TearDown() override;
  void tearDownAgentEnsemble(bool doWarmboot = false);
  using StateUpdateFn = SwSwitch::StateUpdateFn;

 protected:
  template <
      typename SETUP_FN,
      typename VERIFY_FN,
      typename SETUP_POSTWB_FN,
      typename VERIFY_POSTWB_FN>
  void verifyAcrossWarmBoots(
      SETUP_FN setup,
      VERIFY_FN verify,
      SETUP_POSTWB_FN setupPostWarmboot,
      VERIFY_POSTWB_FN verifyPostWarmboot) {
    if (agentEnsemble_->getSw()->getBootType() != BootType::WARM_BOOT) {
      XLOG(DBG2) << "cold boot setup()";
      setup();
    } else {
      XLOG(DBG2) << "skip setup() for warmboot";
    }

    if (runVerification()) {
      XLOG(DBG2) << "verify()";
      verify();
    }

    if (agentEnsemble_->getSw()->getBootType() == BootType::WARM_BOOT) {
      // If we did a warm boot, do post warmboot actions now
      XLOG(DBG2) << "setupPostWarmboot()";
      setupPostWarmboot();

      if (runVerification()) {
        XLOG(DBG2) << "verifyPostWarmboot()";
        verifyPostWarmboot();
      }
    }
    if (FLAGS_setup_for_warmboot &&
        isSupportedOnAllAsics(HwAsic::Feature::WARMBOOT)) {
      XLOG(DBG2) << "tearDownAgentEnsemble() for warmboot";
      tearDownAgentEnsemble(true);
    }
  }

  template <typename SETUP_FN, typename VERIFY_FN>
  void verifyAcrossWarmBoots(SETUP_FN setup, VERIFY_FN verify) {
    verifyAcrossWarmBoots(setup, verify, []() {}, []() {});
  }
  template <typename VERIFY_FN>
  void verifyAcrossWarmBoots(VERIFY_FN verify) {
    verifyAcrossWarmBoots([]() {}, verify, []() {}, []() {});
  }

  void setupPlatformConfig(AgentEnsemblePlatformConfigFn platformConfigFn) {
    platformConfigFn_ = std::move(platformConfigFn);
  }

  void runForever() const;
  std::shared_ptr<SwitchState> applyNewConfig(const cfg::SwitchConfig& config);
  void applyNewState(StateUpdateFn fn, const std::string& name = "agent-test") {
    return applyNewStateImpl(fn, name, false);
  }
  void applyNewStateTransaction(
      StateUpdateFn fn,
      const std::string& name = "agent-test-transaction") {
    return applyNewStateImpl(fn, name, true);
  }

  SwSwitch* getSw() const;
  const std::map<SwitchID, const HwAsic*> getAsics() const;
  const HwAsic& getAsic(SwitchID swId) const {
    return *getAsics().find(swId)->second;
  }
  const SwitchIdScopeResolver& scopeResolver() const {
    return getAgentEnsemble()->scopeResolver();
  }
  bool isSupportedOnAllAsics(HwAsic::Feature feature) const;
  AgentEnsemble* getAgentEnsemble() const;
  const std::shared_ptr<SwitchState> getProgrammedState() const;
  std::vector<PortID> masterLogicalPortIds() const;
  std::vector<PortID> masterLogicalPortIds(
      const std::set<cfg::PortType>& portTypes) const;
  std::vector<PortID> masterLogicalPortIds(
      const std::set<cfg::PortType>& portTypes,
      SwitchID switchId) const;
  std::vector<PortID> masterLogicalInterfacePortIds() const {
    return masterLogicalPortIds({cfg::PortType::INTERFACE_PORT});
  }
  std::vector<PortID> masterLogicalFabricPortIds() const {
    return masterLogicalPortIds({cfg::PortType::FABRIC_PORT});
  }
  std::vector<PortID> masterLogicalFabricPortIds(SwitchID switchId) const {
    return masterLogicalPortIds({cfg::PortType::FABRIC_PORT}, switchId);
  }
  void setSwitchDrainState(
      const cfg::SwitchConfig& curConfig,
      cfg::SwitchDrainState drainState);
  void applySwitchDrainState(cfg::SwitchDrainState drainState);

  std::map<PortID, HwPortStats> getLatestPortStats(
      const std::vector<PortID>& ports);

  HwPortStats getLatestPortStats(const PortID& port);
  std::map<PortID, HwPortStats> getNextUpdatedPortStats(
      const std::vector<PortID>& ports);
  HwPortStats getNextUpdatedPortStats(const PortID& port);
  HwPortStats getLastIncrementedPortStats(const PortID& port);
  std::map<PortID, std::pair<HwPortStats, HwPortStats>>
  sendTrafficAndCollectStats(
      const std::vector<PortID>& ports,
      int timeIntervalInSec,
      const std::function<void()>& startSendFn,
      const std::function<void()>& stopSendFn = []() {},
      bool keepTrafficRunning = false);

  std::map<SystemPortID, HwSysPortStats> getLatestSysPortStats(
      const std::vector<SystemPortID>& ports);

  HwSysPortStats getLatestSysPortStats(const SystemPortID& port);
  std::optional<HwSysPortStats> getLatestCpuSysPortStats();

  HwSwitchDropStats getAggregatedSwitchDropStats();

  std::map<uint16_t, HwSwitchWatermarkStats> getAllSwitchWatermarkStats();

  virtual cfg::SwitchConfig initialConfig(const AgentEnsemble& ensemble) const;

  cfg::SwitchConfig addCoppConfig(
      const AgentEnsemble& ensemble,
      const cfg::SwitchConfig& in) const;

  template <typename EcmpHelperT>
  void resolveNeigborAndProgramRoutes(const EcmpHelperT& ecmp, int width) {
    applyNewState([this, &ecmp, &width](std::shared_ptr<SwitchState> /*in*/) {
      return ecmp.resolveNextHops(getProgrammedState(), width);
    });
    auto wrapper = getSw()->getRouteUpdater();
    ecmp.programRoutes(&wrapper, width);
  }

  template <typename EcmpHelperT>
  void unprogramRoutes(const EcmpHelperT& ecmp) {
    auto wrapper = getSw()->getRouteUpdater();
    ecmp.unprogramRoutes(&wrapper);
  }

  void bringUpPort(PortID port) {
    agentEnsemble_->bringUpPort(port);
  }
  void bringDownPort(PortID port) {
    agentEnsemble_->bringDownPort(port);
  }
  void bringUpPorts(const std::vector<PortID>& ports) {
    agentEnsemble_->bringUpPorts(ports);
  }
  void bringDownPorts(const std::vector<PortID>& ports) {
    agentEnsemble_->bringDownPorts(ports);
  }

  void checkNoStatsChange(int trys = 1);
  /*
   * API to all flag overrides for individual tests. Primarily
   * used for features which we don't want to enable for
   * all tests, but still want to tweak/test this behavior in
   * our test.
   */
  virtual void setCmdLineFlagOverrides() const;

  SwitchID switchIdForPort(PortID port) const;
  const HwAsic* hwAsicForPort(PortID port) const;
  const HwAsic* hwAsicForSwitch(SwitchID switchID) const;

  void populateArpNeighborsToCache(const std::shared_ptr<Interface>& interface);
  void populateNdpNeighborsToCache(const std::shared_ptr<Interface>& interface);

 private:
  void applyNewStateImpl(
      StateUpdateFn fn,
      const std::string& name,
      bool transaction);

  void dumpConfigWithOverriddenGflags(AgentConfig* agentConfig) const;

  /*
   * Derived classes have the option to not run verify on
   * certain DUTs. E.g. non controlling nodes in Multinode setups
   */
  virtual bool runVerification() const {
    return true;
  }

  virtual std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const = 0;
  void printProductionFeatures() const;
  virtual bool failHwCallsOnWarmboot() const {
    return true;
  }

  AgentEnsemblePlatformConfigFn platformConfigFn_ = nullptr;
  std::unique_ptr<AgentEnsemble> agentEnsemble_;
};

void initAgentHwTest(
    int argc,
    char* argv[],
    PlatformInitFn initPlatform,
    std::optional<cfg::StreamType> streamType = std::nullopt);

} // namespace facebook::fboss
