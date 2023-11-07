// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/agent/test/MockAgentCommandExecutor.h"
#include "fboss/agent/test/MockAgentNetWhoAmI.h"

namespace facebook::fboss {

namespace {
template <bool multiSwitch, bool cppRefactor, bool sai, bool brcm>
struct TestAttr {
  static constexpr bool kMultiSwitch = multiSwitch;
  static constexpr bool kCppRefactor = cppRefactor;
  static constexpr bool kSai = sai;
  static constexpr bool kBrcm = brcm;
};

#define TEST_ATTR(NAME, a, b, c, d) \
  struct TestAttr##NAME : public TestAttr<a, b, c, d> {};

TEST_ATTR(NoMultiSwitchNoCppRefactorNoSaiBrcm, false, false, false, true);
TEST_ATTR(NoMultiSwitchNoCppRefactorSaiBrcm, false, false, true, true);
TEST_ATTR(NoMultiSwitchNoCppRefactorSaiNoBrcm, false, false, true, false);

TEST_ATTR(NoMultiSwitchCppRefactorNoSaiBrcm, false, true, false, true);
TEST_ATTR(NoMultiSwitchCppRefactorSaiBrcm, false, true, true, true);
TEST_ATTR(NoMultiSwitchCppRefactorSaiNoBrcm, false, true, true, false);

TEST_ATTR(MultiSwitchNoCppRefactorNoSaiBrcm, true, false, false, true);
TEST_ATTR(MultiSwitchNoCppRefactorSaiBrcm, true, false, true, true);
TEST_ATTR(MultiSwitchNoCppRefactorSaiNoBrcm, true, false, true, false);

TEST_ATTR(MultiSwitchCppRefactorNoSaiBrcm, true, true, false, true);
TEST_ATTR(MultiSwitchCppRefactorSaiBrcm, true, true, true, true);
TEST_ATTR(MultiSwitchCppRefactorSaiNoBrcm, true, true, true, false);
} // namespace

template <typename TestAttr>
class AgentPreStartExecTests : public ::testing::Test {
 public:
  void SetUp() override {}
  void TearDown() override {}
  void run(){};
};

#define TestFixtureName(NAME)                                    \
  class NAME : public AgentPreStartExecTests<TestAttr##NAME> {}; \
  TEST_F(NAME, PreStartExec) {                                   \
    this->run();                                                 \
  }

/* TODO: retire NoCpp refactor subsequently */
TestFixtureName(NoMultiSwitchNoCppRefactorNoSaiBrcm);
TestFixtureName(NoMultiSwitchNoCppRefactorSaiBrcm);
TestFixtureName(NoMultiSwitchNoCppRefactorSaiNoBrcm);

TestFixtureName(NoMultiSwitchCppRefactorNoSaiBrcm);
TestFixtureName(NoMultiSwitchCppRefactorSaiBrcm);
TestFixtureName(NoMultiSwitchCppRefactorSaiNoBrcm);

TestFixtureName(MultiSwitchNoCppRefactorNoSaiBrcm);
TestFixtureName(MultiSwitchNoCppRefactorSaiBrcm);
TestFixtureName(MultiSwitchNoCppRefactorSaiNoBrcm);

TestFixtureName(MultiSwitchCppRefactorNoSaiBrcm);
TestFixtureName(MultiSwitchCppRefactorSaiBrcm);
TestFixtureName(MultiSwitchCppRefactorSaiNoBrcm);

} // namespace facebook::fboss
