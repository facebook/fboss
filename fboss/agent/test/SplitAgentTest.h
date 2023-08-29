// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/test/AgentEnsemble.h"

#include <gtest/gtest.h>

namespace facebook::fboss {

/*
 * SplitAgentTest is the base class for agent tests running in mono or split
 * mode. At early phases, the tests will be running in both mono and split mode
 * to assist migration. This should eventually be named as AgentTest as all
 * Agent/Hw tests are migrated to this framework.
 */

class SplitAgentTest : public ::testing::Test {
 public:
  SplitAgentTest() = default;
  ~SplitAgentTest() override = default;
  void SetUp() override;
  void TearDown() override;

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

 private:
  /*
   * Derived classes have the option to not run verify on
   * certain DUTs. E.g. non controlling nodes in Multinode setups
   */
  virtual bool runVerification() const {
    return true;
  }

  std::unique_ptr<AgentEnsemble> agentEnsemble_;
};
} // namespace facebook::fboss
