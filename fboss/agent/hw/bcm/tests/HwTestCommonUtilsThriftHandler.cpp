// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
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
  auto handler = *logCaptureHandler_.rlock();
  if (!handler) {
    return;
  }
  out = static_cast<CaptureLogHandler*>(handler.get())->matching(*substring);
}

void HwTestThriftHandler::printDiagCmd(std::unique_ptr<::std::string> cmd) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch_);
  bcmSwitch->printDiagCmd(*cmd);
}
void HwTestThriftHandler::getVlanToNumPorts(
    std::map<int32_t, int32_t>& vlanToNumPorts) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch_);
  auto unit = bcmSwitch->getUnit();
  bcm_vlan_data_t* vlanList = nullptr;
  int vlanCount = 0;
  SCOPE_EXIT {
    bcm_vlan_list_destroy(unit, vlanList, vlanCount);
  };
  auto rv = bcm_vlan_list(unit, &vlanList, &vlanCount);
  bcmCheckError(rv, "failed to list all VLANs");
  for (int i = 0; i < vlanCount; i++) {
    int portCount;
    BCM_PBMP_COUNT(vlanList[i].port_bitmap, portCount);
    vlanToNumPorts[static_cast<int32_t>(vlanList[i].vlan_tag)] = portCount;
  }
}

} // namespace utility
} // namespace fboss
} // namespace facebook
