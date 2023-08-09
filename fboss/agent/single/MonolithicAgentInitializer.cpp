// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/single/MonolithicAgentInitializer.h"

#include <folly/MacAddress.h>
#include <folly/ScopeGuard.h>
#include <folly/SocketAddress.h>
#include <folly/String.h>
#include <folly/experimental/FunctionScheduler.h>
#include <folly/io/async/AsyncSignalHandler.h>
#include <folly/io/async/EventBase.h>
#include <folly/logging/Init.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/AlpmUtils.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/CommonInit.h"
#include "fboss/agent/FbossInit.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/RestartTimeTracker.h"
#include "fboss/agent/SetupThrift.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/single/MonolithicHwSwitchHandler.h"
#include "fboss/agent/single/MonolithicHwSwitchHandlerDeprecated.h"

#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/TunManager.h"
#include "fboss/lib/CommonUtils.h"

#include <gflags/gflags.h>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <cstdio>
#include <functional>
#include <future>
#include <mutex>
#include <string>
#ifndef IS_OSS
#if __has_feature(address_sanitizer)
#include <sanitizer/lsan_interface.h>
#endif
#endif

using apache::thrift::ThriftServer;
using folly::AsyncSignalHandler;
using folly::EventBase;
using folly::FunctionScheduler;
using folly::SocketAddress;
using std::condition_variable;
using std::mutex;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::chrono::seconds;
using namespace std::chrono;

using facebook::fboss::SwSwitch;
using facebook::fboss::ThriftHandler;

namespace facebook::fboss {

void MonolithicSwSwitchInitializer::initImpl(
    HwSwitchCallback* hwSwitchCallback) {
  // Initialize the switch.  This operation can take close to a minute
  // on some of our current platforms.
  sw_->init(
      hwSwitchCallback,
      nullptr,
      [this](HwSwitchCallback* callback, bool failHwCallsOnWarmboot) {
        return hwAgent_->initAgent(failHwCallsOnWarmboot, callback);
      },
      setupFlags());
}

MonolithicAgentInitializer::MonolithicAgentInitializer(
    std::unique_ptr<AgentConfig> config,
    uint32_t hwFeaturesDesired,
    PlatformInitFn initPlatform) {
  createSwitch(std::move(config), hwFeaturesDesired, initPlatform);
}

void MonolithicAgentInitializer::createSwitch(
    std::unique_ptr<AgentConfig> config,
    uint32_t hwFeaturesDesired,
    PlatformInitFn initPlatform) {
  // Allow any flag overrides to kick in before we create SwSwitch
  setCmdLineFlagOverrides();

  hwAgent_ = std::make_unique<HwAgent>(
      std::move(config), hwFeaturesDesired, initPlatform, 0 /*switchIndex*/);

  auto platform = hwAgent_->getPlatform();
  CHECK(platform);
  auto hwSwitchHandler =
      std::make_unique<MonolinithicHwSwitchHandlerDeprecated>(platform);

  // Create the SwSwitch and thrift handler
  sw_ = std::make_unique<SwSwitch>(
      std::move(hwSwitchHandler),
      [platform](const SwitchID& switchId, const cfg::SwitchInfo& info) {
        return std::make_unique<MonolithicHwSwitchHandler>(
            platform, switchId, info);
      });
  initializer_ = std::make_unique<MonolithicSwSwitchInitializer>(
      sw_.get(), hwAgent_.get());
}

void MonolithicAgentInitializer::handleExitSignal() {
  SwAgentInitializer::handleExitSignal();
  __attribute__((unused)) auto leakedHwAgent = hwAgent_.release();
#ifndef IS_OSS
#if __has_feature(address_sanitizer)
  __lsan_ignore_object(leakedHwAgent);
#endif
#endif
  exit(0);
}

std::vector<std::shared_ptr<apache::thrift::AsyncProcessorFactory>>
MonolithicAgentInitializer::getThrifthandlers() {
  std::vector<std::shared_ptr<apache::thrift::AsyncProcessorFactory>>
      handlers{};
  auto hwHandler = platform()->createHandler();
  if (hwHandler) {
    handlers.push_back(hwHandler);
  }
  return handlers;
}

} // namespace facebook::fboss
