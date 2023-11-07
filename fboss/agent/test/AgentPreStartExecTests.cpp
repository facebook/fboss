// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/agent/test/MockAgentCommandExecutor.h"
#include "fboss/agent/test/MockAgentNetWhoAmI.h"

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/AgentDirectoryUtil.h"
#include "fboss/agent/AgentPreStartExec.h"
#include "fboss/agent/FbossError.h"
#include "fboss/lib/CommonFileUtils.h"

#include <folly/logging/xlog.h>
#include <cstdio>
#include <filesystem>

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
  void SetUp() override {
    tmpDir_ = folly::to<std::string>(std::tmpnam(nullptr), "_PreStartExec");
    std::string volatileDir = folly::to<std::string>(tmpDir_, "/dev/shm/fboss");
    std::string persistentDir =
        folly::to<std::string>(tmpDir_, "/var/facebook/fboss");
    std::string packageDirectory =
        folly::to<std::string>(tmpDir_, "/packages/wedge_agent/current");
    std::string systemdDir = folly::to<std::string>(tmpDir_, "/systemd/system");
    std::string configDir = folly::to<std::string>(tmpDir_, "/coop/agent");
    std::string drainConfigDir =
        folly::to<std::string>(tmpDir_, "/coop/agent_drain");
    util_ = AgentDirectoryUtil(
        volatileDir,
        persistentDir,
        packageDirectory,
        systemdDir,
        configDir,
        drainConfigDir);
    removeDirectories(tmpDir_);
    setupDirectories();
    touchFile(util_.getAgentLiveConfig());
  }

  void TearDown() override {
    removeDirectories(tmpDir_);
  }

  cfg::AgentConfig getConfig() {
    cfg::AgentConfig config;
    setSdkVersion(config.sw().value());
    if constexpr (TestAttr::kMultiSwitch) {
      // set the multi switch mode in config
      setMultiSwitchMode(config.defaultCommandLineArgs().value());
    }
    setSwitchIdToSwitchInfo(config.sw().value());
    return config;
  }

  void run(bool coldBoot = false) {
    using ::testing::Return;

    MockAgentCommandExecutor executor;
    AgentPreStartExec exec;
    auto netwhoami = std::make_unique<MockAgentNetWhoAmI>();
    ON_CALL(*netwhoami, isSai()).WillByDefault(Return(TestAttr::kSai));
    ON_CALL(*netwhoami, isBcmSaiPlatform())
        .WillByDefault(Return(TestAttr::kBrcm && TestAttr::kSai));
    ON_CALL(*netwhoami, isCiscoSaiPlatform())
        .WillByDefault(Return(!TestAttr::kBrcm));
    ON_CALL(*netwhoami, isBcmPlatform()).WillByDefault(Return(TestAttr::kBrcm));
    ON_CALL(*netwhoami, isCiscoPlatform())
        .WillByDefault(Return(!TestAttr::kBrcm));
    ON_CALL(*netwhoami, isBcmVoqPlatform()).WillByDefault(Return(false));
    ON_CALL(*netwhoami, isFdsw()).WillByDefault(Return(false));
    ON_CALL(*netwhoami, isUnDrainable()).WillByDefault(Return(false));
    ON_CALL(*netwhoami, hasRoutingProtocol()).WillByDefault(Return(false));
    ON_CALL(*netwhoami, hasBgpRoutingProtocol()).WillByDefault(Return(false));
    if (coldBoot) {
      // touch cold_boot_once_0
      createDirectoryTree(util_.getWarmBootDir());
      touchFile(util_.getColdBootOnceFile());
    }
    if (!TestAttr::kCppRefactor) {
      if (TestAttr::kMultiSwitch) {
        EXPECT_CALL(
            executor,
            runShellCommand(util_.getMultiSwitchPreStartScript(), true))
            .Times(1);
      }
    } else {
      ::testing::InSequence seq;
      EXPECT_CALL(*netwhoami, isFdsw()).WillOnce(Return(false));
      EXPECT_CALL(*netwhoami, isFdsw()).WillOnce(Return(false));
      EXPECT_CALL(*netwhoami, isUnDrainable()).WillOnce(Return(false));
      EXPECT_CALL(*netwhoami, isFdsw()).WillOnce(Return(false));
      EXPECT_CALL(*netwhoami, isBcmPlatform())
          .WillOnce(Return(TestAttr::kBrcm));
      if (!TestAttr::kBrcm) {
        EXPECT_CALL(*netwhoami, isCiscoPlatform()).WillOnce(Return(true));
      }

      std::string kmodsInstaller = (TestAttr::kBrcm)
          ? "/usr/local/bin/fboss_bcm_kmods"
          : "/usr/local/bin/fboss_leaba_kmods";
      std::string kmodVersion = getAsicSdkVersion(getSdkVersion());
      std::vector<std::string> installKmod{
          kmodsInstaller, "install", "--sdk-upgrade", kmodVersion};
      EXPECT_CALL(executor, runCommand(installKmod, true)).Times(1);
      EXPECT_CALL(*netwhoami, isBcmSaiPlatform())
          .WillOnce(Return(TestAttr::kSai && TestAttr::kBrcm));
      if (TestAttr::kMultiSwitch) {
        // once again for hw agent
        EXPECT_CALL(*netwhoami, isBcmSaiPlatform())
            .WillOnce(Return(TestAttr::kSai && TestAttr::kBrcm));
      }
      EXPECT_CALL(*netwhoami, isBcmVoqPlatform()).WillOnce(Return(false));
      if (TestAttr::kMultiSwitch) {
        EXPECT_CALL(
            executor,
            runCommand(
                std::vector<std::string>{
                    "/usr/bin/systemctl",
                    "enable",
                    util_.getSwAgentServicePath()},
                true));
        EXPECT_CALL(
            executor,
            runCommand(
                std::vector<std::string>{
                    "/usr/bin/systemctl",
                    "enable",
                    util_.getHwAgentServiceInstance(0)},
                true));

        EXPECT_CALL(
            executor,
            runCommand(
                std::vector<std::string>{
                    "/usr/bin/systemctl",
                    "enable",
                    util_.getHwAgentServiceInstance(1)},
                true));
      }
      // update-buildinfo
      EXPECT_CALL(
          executor, runShellCommand("/usr/local/bin/fboss-build-info", false))
          .Times(1);
      if (TestAttr::kMultiSwitch) {
        EXPECT_CALL(
            executor,
            runShellCommand(util_.getMultiSwitchPreStartScript(), true))
            .Times(1);
      }
    }

    exec.run(
        &executor,
        std::move(netwhoami),
        util_,
        std::make_unique<AgentConfig>(getConfig()),
        TestAttr::kCppRefactor);

    if (TestAttr::kCppRefactor) {
      auto verifySymLink = [&](const std::string& name,
                               const std::string& sdk) {
        auto agentSymLink = util_.getPackageDirectory() + "/" + name;
        auto actualTarget = std::filesystem::read_symlink(agentSymLink);
        auto expectedTarget =
            util_.getPackageDirectory() + "/" + sdk + "/" + name;
        EXPECT_EQ(actualTarget.string(), expectedTarget);
      };
      verifySymLink(
          "wedge_agent",
          // sai + brcm has directories with sai sdk version
          // non-sai + brcm and non-brcm has directories with asic sdk version
          TestAttr::kSai && TestAttr::kBrcm
              ? getSaiSdkVersion(getSdkVersion())
              : getAsicSdkVersion(getSdkVersion()));

      if (coldBoot) {
        EXPECT_TRUE(checkFileExists(util_.getSwColdBootOnceFile()));
        auto cfg = getConfig();
        for (auto [id, info] :
             *(cfg.sw()->switchSettings()->switchIdToSwitchInfo())) {
          EXPECT_TRUE(checkFileExists(
              util_.getHwColdBootOnceFile(info.switchIndex().value())));
        }
        EXPECT_FALSE(checkFileExists(util_.getColdBootOnceFile()));
      }
    }
  }

 private:
  void setSdkVersion(cfg::SwitchConfig& config) {
    config.sdkVersion() = getSdkVersion();
  }

  cfg::SdkVersion getSdkVersion() {
    cfg::SdkVersion sdkVersion;
    sdkVersion.asicSdk() = "asic_1.0";
    if constexpr (TestAttr::kSai && TestAttr::kBrcm) {
      sdkVersion.saiSdk() = "sai_1.1";
    }
    return sdkVersion;
  }

  std::string getSaiSdkVersion(const cfg::SdkVersion& sdkVersion) {
    if (!sdkVersion.saiSdk().has_value()) {
      throw FbossError("SAI SDK version not found");
    }
    return sdkVersion.saiSdk().value();
  }

  std::string getAsicSdkVersion(const cfg::SdkVersion& sdkVersion) {
    if (!sdkVersion.asicSdk().has_value()) {
      throw FbossError("Asic SDK version not found");
    }
    return sdkVersion.asicSdk().value();
  }

  void setMultiSwitchMode(std::map<std::string, std::string>& args) {
    args.emplace("multi_switch", TestAttr::kMultiSwitch ? "true" : "false");
  }

  void setSwitchIdToSwitchInfo(cfg::SwitchConfig& config) {
    std::map<int64_t, cfg::SwitchInfo> switchIdToSwitchInfo;
    auto numSwitches = TestAttr::kMultiSwitch ? 2 : 1;
    for (auto i = 0; i < numSwitches; ++i) {
      switchIdToSwitchInfo.emplace(i, getSwitchInfo(i));
    }
    config.switchSettings()->switchIdToSwitchInfo() = switchIdToSwitchInfo;
  }

  cfg::SwitchInfo getSwitchInfo(int64_t id) {
    cfg::SwitchInfo info;
    info.switchType() = cfg::SwitchType::FABRIC;
    info.asicType() = cfg::AsicType::ASIC_TYPE_MOCK;
    info.switchIndex() = id;
    return info;
  }

  void createDirectory(const std::string& path) {
    XLOG(INFO) << "Creating directory : " << path;
    createDirectoryTree(path);
  }

  void removeDirectories(const std::string& path) {
    XLOG(INFO) << "Removing directory : " << path;
    std::filesystem::remove_all(path);
  }

  void setupDirectories() {
    createDirectory(util_.getPackageDirectory());
    if (TestAttr::kMultiSwitch) {
      createDirectory(util_.getMultiSwitchScriptsDirectory());
      touchFile(util_.getSwAgentServicePath());
      touchFile(util_.getHwAgentServiceTemplatePath());
    }
    createDirectory(util_.getSystemdDirectory());
    createDirectory(util_.getConfigDirectory());
    createDirectory(util_.getDrainConfigDirectory());
    auto sdkVersion = getSdkVersion();
    std::string binDir = util_.getPackageDirectory() + "/" +
        ((TestAttr::kSai && TestAttr::kBrcm) ? getSaiSdkVersion(sdkVersion)
                                             : getAsicSdkVersion(sdkVersion));
    createDirectoryTree(binDir);
    touchFile(binDir + "/wedge_agent");
    touchFile(binDir + "/wedge_hwagent");
    touchFile(util_.getPackageDirectory() + "/fboss_sw_agent");
  }

  AgentDirectoryUtil getAgentDirectoryUtil() {}
  AgentDirectoryUtil util_;
  std::string tmpDir_;
};

#define TestFixtureName(NAME)                                    \
  class NAME : public AgentPreStartExecTests<TestAttr##NAME> {}; \
  TEST_F(NAME, PreStartExec) {                                   \
    run();                                                       \
  }                                                              \
  TEST_F(NAME, PreStartExecColdBoot) {                           \
    run(true);                                                   \
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
