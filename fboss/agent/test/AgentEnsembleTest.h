// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Main.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/test/AgentEnsemble.h"

#include <gtest/gtest.h>

namespace facebook::fboss {
class SwitchState;

class AgentEnsembleTest : public ::testing::Test {
 public:
  void SetUp() override {
    setupAgentEnsemble();
  }
  void TearDown() override;
  void tearDownAgentEnsemble(bool doWarmboot = false);
  using StateUpdateFn = SwSwitch::StateUpdateFn;

 protected:
  void setupAgentEnsemble();
  void runForever() const;
  std::map<PortID, HwPortStats> getPortStats(
      const std::vector<PortID>& ports) const;

  void resolveNeighbor(
      PortDescriptor port,
      const folly::IPAddress& ip,
      folly::MacAddress mac);
  bool waitForSwitchStateCondition(
      std::function<bool(const std::shared_ptr<SwitchState>&)> conditionFn,
      uint32_t retries = 10,
      std::chrono::duration<uint32_t, std::milli> msBetweenRetry =
          std::chrono::milliseconds(1000)) const;
  void setPortStatus(PortID port, bool up);
  void setPortLoopbackMode(PortID port, cfg::PortLoopbackMode mode);
  std::string getPortName(PortID portId) const;
  std::vector<std::string> getPortNames(const std::vector<PortID>& ports) const;
  void waitForLinkStatus(
      const std::vector<PortID>& portsToCheck,
      bool up,
      uint32_t retries = 60,
      std::chrono::duration<uint32_t, std::milli> msBetweenRetry =
          std::chrono::milliseconds(1000)) const;
  /*
   * Assert no in discards occured on any of the switch ports.
   * When used in conjunction with createL3DataplaneFlood, can be
   * used to verify that none of the traffic bearing ports flapped
   */
  void assertNoInDiscards(int maxNumDiscards = 0);
  void assertNoInErrors(int maxNumDiscards = 0);

  void getAllHwPortStats(std::map<std::string, HwPortStats>& hwPortStats) const;

  void reloadPlatformConfig();
  std::map<PortID, FabricEndpoint> getFabricConnectivity(
      SwitchID switchId) const;

  void applyNewState(
      const StateUpdateFn& fn,
      const std::string& name = "agent-ensemble-test") {
    return applyNewStateImpl(fn, name, false);
  }

  void applyNewStateImpl(
      const StateUpdateFn& fn,
      const std::string& name,
      bool transaction) {
    agentEnsemble_->applyNewState(fn, name, transaction);
  }

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

  std::string getAgentTestDir() const {
    return AgentDirectoryUtil().agentEnsembleConfigDir();
  }

  std::string getTestConfigPath() const {
    return getAgentTestDir() + "/agent.conf";
  }

  std::shared_ptr<SwitchState> applyNewConfig(const cfg::SwitchConfig& config);

  void dumpRunningConfig(const std::string& targetDir);
  SwSwitch* getSw() const;
  void setupConfigFile(
      const cfg::SwitchConfig& cfg,
      const std::string& configDir,
      const std::string& configFile) const;
  void reloadConfig(std::string reason) const;
  virtual void logLinkDbgMessage(std::vector<PortID>& /* portIDs */) const {}

  SwitchID scope(const boost::container::flat_set<PortDescriptor>& ports);
  SwitchID scope(
      const std::shared_ptr<SwitchState>& state,
      const boost::container::flat_set<PortDescriptor>& ports);

  PortID getPortID(const std::string& portName) const;

  virtual void setCmdLineFlagOverrides() const;

  void setupPlatformConfig(AgentEnsemblePlatformConfigFn platformConfigFn) {
    platformConfigFn_ = std::move(platformConfigFn);
  }
  virtual cfg::SwitchConfig initialConfig(const AgentEnsemble& ensemble);
  virtual void preInitSetup();
  bool isSupportedOnAllAsics(HwAsic::Feature feature) const;
  AgentEnsemble* getAgentEnsemble() const;
  const std::shared_ptr<SwitchState> getProgrammedState() const;
  std::map<std::string, HwPortStats> getNextUpdatedHwPortStats(
      int64_t timestamp);

  AgentEnsemblePlatformConfigFn platformConfigFn_ = nullptr;

 private:
  template <typename AddrT>
  void resolveNeighbor(
      PortDescriptor port,
      const AddrT& ip,
      VlanID vlan,
      folly::MacAddress mac);
  /*
   * Derived classes have the option to not run verify no
   * certain DUTs. E.g. non controlling nodes in Multinode setups
   */
  virtual bool runVerification() const {
    return true;
  }
  std::unique_ptr<AgentEnsemble> agentEnsemble_;
};

void initAgentEnsembleTest(
    int argc,
    char** argv,
    PlatformInitFn initPlatformFn,
    std::optional<cfg::StreamType> streamType = std::nullopt);
} // namespace facebook::fboss
