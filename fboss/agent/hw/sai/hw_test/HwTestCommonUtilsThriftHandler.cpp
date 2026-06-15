// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"

#include <folly/Synchronized.h>
#include <folly/logging/LogHandler.h>
#include <folly/logging/LogHandlerConfig.h>
#include <folly/logging/LogMessage.h>
#include <folly/logging/LoggerDB.h>

namespace facebook {
namespace fboss {
namespace utility {

namespace {
// Thread-safe in-memory log capture. folly::TestLogHandler is explicitly NOT
// thread-safe (unguarded vector append in handleMessage), and the hw_agent
// logs from many threads concurrently, so installing it on the root category
// races and corrupts memory. Synchronize the message store instead.
class CaptureLogHandler : public folly::LogHandler {
 public:
  void handleMessage(
      const folly::LogMessage& message,
      const folly::LogCategory* /*category*/) override {
    messages_.wlock()->push_back(message.getMessage());
  }
  void flush() override {}
  folly::LogHandlerConfig getConfig() const override {
    return folly::LogHandlerConfig{"capture"};
  }
  std::vector<std::string> matching(const std::string& substring) const {
    // Snapshot under the lock, then filter lock-free. matching() must not hold
    // the read lock during the scan: handleMessage takes the write lock on
    // every log line, so a long scan would stall all logging threads.
    auto snapshot = messages_.copy();
    std::vector<std::string> out;
    for (const auto& m : snapshot) {
      if (m.find(substring) != std::string::npos) {
        out.push_back(m);
      }
    }
    return out;
  }
  void clear() {
    messages_.wlock()->clear();
  }

 private:
  folly::Synchronized<std::vector<std::string>> messages_;
};
} // namespace

void HwTestThriftHandler::installLogCapture() {
  // Hold the member lock across check-and-install so concurrent RPCs can't
  // both pass the null check and add two handlers to the root category. On
  // re-install, clear the existing buffer so each call starts a fresh capture
  // window (also bounds memory to one window's worth of messages).
  auto locked = logCaptureHandler_.wlock();
  if (*locked) {
    static_cast<CaptureLogHandler*>(locked->get())->clear();
    return;
  }
  auto handler = std::make_shared<CaptureLogHandler>();
  folly::LoggerDB::get().getCategory("")->addHandler(handler);
  *locked = std::move(handler);
}

void HwTestThriftHandler::getMatchingLogMessages(
    std::vector<std::string>& out,
    std::unique_ptr<std::string> substring) {
  // Copy the shared_ptr out under the lock, then scan without holding it so
  // a concurrent install isn't blocked by a long match.
  auto handler = *logCaptureHandler_.rlock();
  if (!handler) {
    return;
  }
  out = static_cast<CaptureLogHandler*>(handler.get())->matching(*substring);
}

void HwTestThriftHandler::printDiagCmd(std::unique_ptr<::std::string> cmd) {
  auto saiSwitch = static_cast<const SaiSwitch*>(hwSwitch_);
  saiSwitch->printDiagCmd(*cmd);
}
void HwTestThriftHandler::getVlanToNumPorts(
    std::map<int32_t, int32_t>& vlanToNumPorts) {
  const auto& vlanMgr =
      static_cast<const SaiSwitch*>(hwSwitch_)->managerTable()->vlanManager();
  for (const auto& [vlanId, handle] : vlanMgr.getVlanHandles()) {
    vlanToNumPorts[static_cast<int32_t>(vlanId)] = handle->vlanMembers.size();
  }
}

} // namespace utility
} // namespace fboss
} // namespace facebook
