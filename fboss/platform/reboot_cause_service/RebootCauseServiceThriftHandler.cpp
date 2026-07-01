// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/reboot_cause_service/RebootCauseServiceThriftHandler.h"

#include "fboss/lib/LogThriftCall.h"

namespace facebook::fboss::platform::reboot_cause_service {

void RebootCauseServiceThriftHandler::getLastRebootCause(
    reboot_cause_service::RebootCauseResult& result) {
  auto log = LOG_THRIFT_CALL(DBG1);
  result = serviceImpl_->getLastRebootCause();
}

void RebootCauseServiceThriftHandler::getRebootCauseHistory(
    std::vector<reboot_cause_service::RebootCauseResult>& result) {
  auto log = LOG_THRIFT_CALL(DBG1);
  result = serviceImpl_->getRebootCauseHistory();
}

} // namespace facebook::fboss::platform::reboot_cause_service
