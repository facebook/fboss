// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fb303/ServiceData.h>
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/HwSwitchFb303Stats.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/lib/CommonUtils.h"
#include "folly/experimental/TestUtil.h"

namespace facebook::fboss {
class HwVoqSwitchInterruptTest : public HwLinkStateDependentTest {
 public:
  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::TAM_NOTIFY};
  }
  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes(),
        true /*interfaceHasSubnet*/);
  }

 protected:
  void runCint(const std::string cintStr) {
    folly::test::TemporaryFile file;
    XLOG(INFO) << " Cint file " << file.path().c_str();
    folly::writeFull(file.fd(), cintStr.c_str(), cintStr.size());
    std::string out;
    getHwSwitchEnsemble()->runDiagCommand(
        folly::sformat("cint {}\n", file.path().c_str()), out);
    getHwSwitchEnsemble()->runDiagCommand("quit\n", out);
  }
};

TEST_F(HwVoqSwitchInterruptTest, allReassemblyContextsTakenError) {
  auto verify = [=, this]() {
    std::string out;
    getHwSwitchEnsemble()->runDiagCommand(
        "modreg RQP_PKT_REAS_INTERRUPT_REGISTER_TEST PKT_REAS_INTERRUPT_REGISTER_TEST=0x8000\n",
        out);
    getHwSwitchEnsemble()->runDiagCommand(
        "modreg RQP_PKT_REAS_INTERRUPT_REGISTER_TEST PKT_REAS_INTERRUPT_REGISTER_TEST=0x10000\n",
        out);
    getHwSwitchEnsemble()->runDiagCommand("quit\n", out);
    WITH_RETRIES({
      getHwSwitch()->updateStats();
      fb303::ThreadCachedServiceData::get()->publishStats();
      auto allReassemblyContextsTaken = getHwSwitch()
                                            ->getSwitchStats()
                                            ->getHwAsicErrors()
                                            .allReassemblyContextsTaken()
                                            .value_or(0);
      XLOG(INFO) << " All Reassemble contexts taken: "
                 << allReassemblyContextsTaken;
      EXPECT_EVENTUALLY_GE(allReassemblyContextsTaken, 1);
      EXPECT_EVENTUALLY_GE(
          getHwSwitch()->getSwitchStats()->getAllReassemblyContextsTakenError(),
          1);
    });
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

} // namespace facebook::fboss
