// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <memory>

#include "fboss/platform/reboot_cause_service/RebootCauseServiceImpl.h"
#include "fboss/platform/reboot_cause_service/if/gen-cpp2/RebootCauseService.h"

namespace facebook::fboss::platform::reboot_cause_service {

class RebootCauseServiceThriftHandler : public RebootCauseServiceSvIf {
 public:
  explicit RebootCauseServiceThriftHandler(
      std::shared_ptr<RebootCauseServiceImpl> serviceImpl)
      : serviceImpl_(std::move(serviceImpl)) {}

  void getLastRebootCause(
      reboot_cause_service::RebootCauseResult& result) override;

 private:
  std::shared_ptr<RebootCauseServiceImpl> serviceImpl_;
};

} // namespace facebook::fboss::platform::reboot_cause_service
