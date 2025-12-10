// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentWrapperTest.h"

#include "fboss/agent/AgentCommandExecutor.h"
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"
#include "tupperware/agent/system/systemd/Service.h"

#include <folly/FileUtil.h>
#include <folly/init/Init.h>
#include <folly/logging/Init.h>
#include <folly/logging/xlog.h>

#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/AgentNetWhoAmI.h"
#include "fboss/lib/CommonFileUtils.h"

#include "fboss/agent/AgentConfig.h"

#include <filesystem>
#include <thread>
#include <type_traits>

DEFINE_int32(num_retries, 5, "number of retries for agent to start");
DEFINE_int32(wait_timeout, 15, "number of seconds to wait before retry");

namespace facebook::fboss {

namespace {

template <bool cppWrapper = false, bool multiSwitch = false>
struct Wrapper {
  static constexpr bool kCppWrapper = cppWrapper;
  static constexpr bool kMultiSwitch = multiSwitch;
};
using CppWrapper = Wrapper<true, false>;
using CppMultiSwitchWrapper = Wrapper<true, true>;
using TestTypes = ::testing::Types<CppWrapper, CppMultiSwitchWrapper>;
} // namespace

template <typename T>
void AgentWrapperTest<T>::SetUp() {
  whoami_ = std::make_unique<AgentNetWhoAmI>();
  if constexpr (!T::kCppWrapper) {
    if (whoami_->isChenabPlatform()) {
      GTEST_SKIP() << "Chenab platform will have only cpp wrapper";
    }
  }
  config_ = AgentConfig::fromFile("/etc/coop/agent/current");

  auto newConfigThrift = config_->thrift;
  newConfigThrift.defaultCommandLineArgs()["agent_graceful_exit_timeout_ms"] =
      "130000";
  createDirectoryTree(util_.getWarmBootDir());

  if constexpr (T::kMultiSwitch) {
    newConfigThrift.defaultCommandLineArgs()["multi_switch"] = "true";
    newConfigThrift.defaultCommandLineArgs()["multi_npu_platform_mapping"] =
        "true";
  }
  config_ = std::make_unique<AgentConfig>(newConfigThrift);
  std::filesystem::copy_options opt =
      std::filesystem::copy_options::overwrite_existing;
  std::filesystem::copy(
      "/etc/coop/agent/current", "/etc/coop/agent/current.backup", opt);
  config_->dumpConfig("/etc/coop/agent/current");
}

template <typename T>
void AgentWrapperTest<T>::TearDown() {
  stop();
  if constexpr (T::kMultiSwitch) {
    std::filesystem::copy_options opt =
        std::filesystem::copy_options::overwrite_existing;
    std::filesystem::copy(
        "/etc/coop/agent/current.backup", "/etc/coop/agent/current", opt);
  }
  removeDir(util_.getWarmBootDir());
}
template <typename T>
bool AgentWrapperTest<T>::isMultiSwitch() const {
  return T::kMultiSwitch;
}

template <typename T>
void AgentWrapperTest<T>::start() {
  AgentCommandExecutor executor;
  executor.startService("wedge_agent");
}

template <typename T>
void AgentWrapperTest<T>::stop() {
  AgentCommandExecutor executor;
  executor.stopService("wedge_agent");
}

template <typename T>
void AgentWrapperTest<T>::wait(bool started) {
  if (started) {
    waitForStart();
  } else {
    waitForStop();
  }
}

template <typename T>
void AgentWrapperTest<T>::waitForStart(const std::string& unit) {
  WITH_RETRIES_N_TIMED(
      FLAGS_num_retries, std::chrono::seconds(FLAGS_wait_timeout), {
        facebook::tupperware::systemd::Service service{unit};
        auto status = service.getStatus();
        EXPECT_EVENTUALLY_EQ(
            status.value().serviceState,
            facebook::tupperware::systemd::ProcessStatus::ServiceState::
                RUNNING);
      });

  auto client = utils::createWedgeAgentClient();
  apache::thrift::RpcOptions options;
  options.setTimeout(std::chrono::seconds(1));
  WITH_RETRIES_N_TIMED(
      FLAGS_num_retries, std::chrono::seconds(FLAGS_wait_timeout), {
        SwitchRunState runState = SwitchRunState::UNINITIALIZED;
        try {
          runState = client->sync_getSwitchRunState(options);
        } catch (const std::exception&) {
          XLOG(INFO) << "Waiting for wedge agent to start";
          continue;
        }
        EXPECT_EVENTUALLY_EQ(runState, SwitchRunState::CONFIGURED);
      });
}

template <typename T>
void AgentWrapperTest<T>::waitForStart() {
  if constexpr (T::kMultiSwitch) {
    waitForStart("fboss_sw_agent.service");
    // add support for other hw_agent instances
    waitForStart("fboss_hw_agent@0.service");
  } else {
    waitForStart("wedge_agent.service");
  }
}

template <typename T>
void AgentWrapperTest<T>::waitForStop(const std::string& unit, bool crash) {
  bool wrapperRefactored = FLAGS_cpp_wedge_agent_wrapper;
  constexpr auto multiSwitch = T::kMultiSwitch;
  WITH_RETRIES_N_TIMED(
      FLAGS_num_retries, std::chrono::seconds(FLAGS_wait_timeout), {
        facebook::tupperware::systemd::Service service{unit};
        auto status = service.waitForExit(
            std::chrono::microseconds(FLAGS_wait_timeout * 1000000));
        EXPECT_EVENTUALLY_EQ(
            status.value().serviceState,
            facebook::tupperware::systemd::ProcessStatus::ServiceState::EXITED);
        if (crash) {
          if (wrapperRefactored) {
            EXPECT_EVENTUALLY_EQ(status.value().exitStatus, 134);
          } else {
            if (multiSwitch) {
              EXPECT_EVENTUALLY_EQ(status.value().exitStatus, 134);
            } else {
              EXPECT_EVENTUALLY_EQ(status.value().exitStatus, 255);
            }
          }
        }
      });
}

template <typename T>
void AgentWrapperTest<T>::waitForStop(bool crash) {
  if constexpr (T::kMultiSwitch) {
    waitForStop("fboss_sw_agent.service", crash);
    // TODO: wait for hw_agent as well
  } else {
    waitForStop("wedge_agent.service", crash);
  }
}

template <typename T>
BootType AgentWrapperTest<T>::getBootType() {
  auto client = utils::createWedgeAgentClient();
  apache::thrift::RpcOptions options;
  options.setTimeout(std::chrono::seconds(1));
  return client->sync_getBootType(options);
}

template <typename T>
pid_t AgentWrapperTest<T>::getAgentPid(const std::string& agentName) const {
  std::string pidStr;
  auto pidFile = util_.pidFile(agentName);
  if (!checkFileExists(pidFile)) {
    throw FbossError(pidFile, " not found");
  }
  folly::readFile(util_.pidFile(agentName).c_str(), pidStr);
  return folly::to<pid_t>(pidStr);
}

template <typename T>
std::optional<std::string> AgentWrapperTest<T>::getCoreDirectory(
    const std::string& agentName,
    pid_t pid) const {
  auto fbossCores = std::filesystem::path("/var/tmp/cores/fboss-cores/");

  if (!std::filesystem::exists(fbossCores) ||
      !std::filesystem::is_directory(fbossCores)) {
    return std::nullopt;
  }

  for (const auto& entry : std::filesystem::directory_iterator(fbossCores)) {
    if (entry.path().string().find(agentName) != std::string::npos &&
        entry.path().string().find(folly::to<std::string>(pid)) !=
            std::string::npos) {
      return entry.path().string();
    }
  }

  return std::nullopt;
}

template <typename T>
std::string AgentWrapperTest<T>::getCoreFile(
    const std::string& directory) const {
  return directory + "/core";
}

template <typename T>
std::string AgentWrapperTest<T>::getCoreMetaData(
    const std::string& directory) const {
  return directory + "/metadata";
}

template <typename T>
std::vector<std::string> AgentWrapperTest<T>::getDrainFiles() const {
  std::vector<std::string> drainFiles;
  drainFiles.push_back(this->util_.getRoutingProtocolColdBootDrainTimeFile());

  if (T::kMultiSwitch) {
    auto iter = this->config_->thrift.sw()
                    ->switchSettings()
                    ->switchIdToSwitchInfo()
                    ->begin();
    while (iter !=
           this->config_->thrift.sw()
               ->switchSettings()
               ->switchIdToSwitchInfo()
               ->end()) {
      auto switchIndex = *(iter->second.switchIndex());
      drainFiles.push_back(
          this->util_.getRoutingProtocolColdBootDrainTimeFile(switchIndex));
      iter++;
    }
  }
  return drainFiles;
}
template <typename T>
void AgentWrapperTest<T>::setupDrainFiles() {
  std::vector<char> data = {'0', '5'};
  for (const auto& file : this->getDrainFiles()) {
    if (!this->whoami_->isNotDrainable() && !this->whoami_->isFdsw()) {
      touchFile(file);
      folly::writeFile(data, file.c_str());
    }
  }
}

template <typename T>
void AgentWrapperTest<T>::cleanupDrainFiles() {
  for (const auto& file : this->getDrainFiles()) {
    removeFile(file);
  }
}

template <typename T>
bool AgentWrapperTest<T>::isSai() const {
  if (whoami_->isTajoSaiPlatform()) {
    return true;
  }
  if (auto sdkVersion = config_->thrift.sw()->sdkVersion()) {
    return sdkVersion->saiSdk().has_value();
  }
  throw FbossError("No sdkVersion found in config");
}

template <typename T>
bool AgentWrapperTest<T>::skipTest() const {
  if (T::kMultiSwitch && !isSai()) {
    // multi-switch is only for SAI
    return true;
  }
  return false;
}

TYPED_TEST_SUITE(AgentWrapperTest, TestTypes);

TYPED_TEST(AgentWrapperTest, ColdBootStartAndStop) {
  if (this->skipTest()) {
    GTEST_SKIP();
    return;
  }
  SCOPE_EXIT {
    this->cleanupDrainFiles();
    removeFile(this->util_.getUndrainedFlag());
    removeFile(this->util_.getColdBootOnceFile());
  };
  this->setupDrainFiles();
  touchFile(this->util_.getColdBootOnceFile());
  touchFile(this->util_.getUndrainedFlag());
  this->start();
  this->waitForStart();
  if (!this->whoami_->isNotDrainable() && !this->whoami_->isFdsw()) {
    // @lint-ignore CLANGTIDY
    for (auto file : this->getDrainFiles()) {
      EXPECT_FALSE(checkFileExists(file));
    }
  }
  EXPECT_EQ(this->getBootType(), BootType::COLD_BOOT);
  this->stop();
  this->waitForStop();
}

TYPED_TEST(AgentWrapperTest, StartAndStopAndStart) {
  if (this->skipTest()) {
    GTEST_SKIP();
    return;
  }
  SCOPE_EXIT {
    removeFile(this->util_.getColdBootOnceFile());
  };
  touchFile(this->util_.getColdBootOnceFile());
  this->start();
  this->waitForStart();
  EXPECT_EQ(this->getBootType(), BootType::COLD_BOOT);
  this->stop();
  this->waitForStop();
  EXPECT_FALSE(checkFileExists(this->util_.getColdBootOnceFile()));
  checkFileExists(this->util_.exitTimeFile("wedge_agent"));
  this->start();
  this->waitForStart();
  EXPECT_EQ(this->getBootType(), BootType::WARM_BOOT);
  this->stop();
  this->waitForStop();
  checkFileExists(this->util_.restartDurationFile("wedge_agent"));
}

TYPED_TEST(AgentWrapperTest, StartAndCrash) {
  if (this->skipTest()) {
    GTEST_SKIP();
    return;
  }
  SCOPE_EXIT {
    removeFile(this->util_.sleepSwSwitchOnSigTermFile());
    removeFile(this->util_.getMaxPostSignalWaitTimeFile());
    runCommand({"/usr/bin/systemctl", "start", "analyze_fboss_cores.timer"});
  };
  runCommand({"/usr/bin/systemctl", "stop", "analyze_fboss_cores.timer"});
  this->start();
  this->waitForStart();
  touchFile(this->util_.sleepSwSwitchOnSigTermFile());
  std::vector<char> sleepTime = {'3', '0', '0'};
  folly::writeFile(sleepTime, this->util_.sleepSwSwitchOnSigTermFile().c_str());
  auto maxPostSignalWaitTime = this->util_.getMaxPostSignalWaitTimeFile();
  touchFile(maxPostSignalWaitTime);
  std::vector<char> data = {'1'};
  folly::writeFile(data, maxPostSignalWaitTime.c_str());
  auto agent = this->isMultiSwitch() ? "fboss_sw_agent" : "wedge_agent";
  auto pid = this->getAgentPid(agent);
  this->stop();
  this->waitForStop(true /* expect sigabrt to crash */);
  // core copier should copy cores here, analyze fboss core timer will remove
  // these
  WITH_RETRIES_N_TIMED(
      FLAGS_num_retries, std::chrono::seconds(FLAGS_wait_timeout), {
        auto coreDir = this->getCoreDirectory(agent, pid);
        ASSERT_EVENTUALLY_TRUE(coreDir.has_value());
        auto coreFile = this->getCoreFile(*coreDir);
        auto coreMetaData = this->getCoreMetaData(*coreDir);
        EXPECT_EVENTUALLY_TRUE(checkFileExists(coreFile));
        EXPECT_EVENTUALLY_TRUE(checkFileExists(coreMetaData));
      });
}

TYPED_TEST(AgentWrapperTest, StartStopRemoveHwSwitchWarmBoot) {
  if (this->skipTest()) {
    GTEST_SKIP();
    return;
  }
  SCOPE_EXIT {
    removeFile(this->util_.getColdBootOnceFile());
    removeFile(this->util_.getUndrainedFlag());
  };
  touchFile(this->util_.getColdBootOnceFile());
  this->start();
  this->waitForStart();
  EXPECT_EQ(this->getBootType(), BootType::COLD_BOOT);
  this->stop();
  this->waitForStop();
  EXPECT_FALSE(checkFileExists(this->util_.getColdBootOnceFile()));
  EXPECT_TRUE(checkFileExists(this->util_.getSwSwitchCanWarmBootFile()));
  EXPECT_TRUE(checkFileExists(this->util_.getHwSwitchCanWarmBootFile(0)));
  removeFile(this->util_.getSwSwitchCanWarmBootFile());
  removeFile(this->util_.getHwSwitchCanWarmBootFile(0));

  this->setupDrainFiles();
  touchFile(this->util_.getUndrainedFlag());
  this->start();
  this->waitForStart();
  if (!this->whoami_->isNotDrainable() && !this->whoami_->isFdsw()) {
    // @lint-ignore CLANGTIDY
    for (auto file : this->getDrainFiles()) {
      EXPECT_FALSE(checkFileExists(file));
    }
  }
  EXPECT_EQ(this->getBootType(), BootType::COLD_BOOT);
}
} // namespace facebook::fboss

#ifdef IS_OSS
FOLLY_INIT_LOGGING_CONFIG("DBG2; default:async=true");
#else
FOLLY_INIT_LOGGING_CONFIG("fboss=DBG4; default:async=true");
#endif

int main(int argc, char* argv[]) {
  // Parse command line flags
  testing::InitGoogleTest(&argc, argv);
  folly::Init init(&argc, &argv, true);

  // Run the tests
  return RUN_ALL_TESTS();
}
