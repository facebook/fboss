// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/FbossInit.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/gen-cpp2/production_features_types.h"

#include <gflags/gflags.h>
#include <gtest/gtest.h>

DECLARE_int32(update_watermark_stats_interval_s);

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
    verifyAcrossWarmBoots(
        setup, verify, []() {}, []() {});
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

  SwSwitch* getSw() const;
  const std::map<SwitchID, const HwAsic*> getAsics() const;
  bool isSupportedOnAllAsics(HwAsic::Feature feature) const;
  AgentEnsemble* getAgentEnsemble() const;
  const std::shared_ptr<SwitchState> getProgrammedState() const;
  std::vector<PortID> masterLogicalPortIds() const;
  std::vector<PortID> masterLogicalPortIds(
      const std::set<cfg::PortType>& portTypes) const;
  std::vector<PortID> masterLogicalInterfacePortIds() const {
    return masterLogicalPortIds({cfg::PortType::INTERFACE_PORT});
  }
  void setSwitchDrainState(
      const cfg::SwitchConfig& curConfig,
      cfg::SwitchDrainState drainState);

 private:
  /*
   * Derived classes have the option to not run verify on
   * certain DUTs. E.g. non controlling nodes in Multinode setups
   */
  virtual bool runVerification() const {
    return true;
  }

  virtual bool hideFabricPorts() const;

  virtual cfg::SwitchConfig initialConfig(const AgentEnsemble& ensemble) const;

  virtual std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const = 0;
  void printProductionFeatures() const;

  AgentEnsemblePlatformConfigFn platformConfigFn_ = nullptr;
  std::unique_ptr<AgentEnsemble> agentEnsemble_;
};

void initAgentHwTest(int argc, char* argv[], PlatformInitFn initPlatform);

} // namespace facebook::fboss
