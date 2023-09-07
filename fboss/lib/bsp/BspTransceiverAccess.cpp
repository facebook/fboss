// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/BspTransceiverAccess.h"

#include <thrift/lib/cpp/util/EnumUtils.h>
#include "fboss/lib/bsp/BspTransceiverCpldAccess.h"
// LibGpiod is not yet supported with OSS - See T126740581
#ifndef IS_OSS
#include "fboss/lib/bsp/BspTransceiverGpioAccess.h"
#endif
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

namespace facebook {
namespace fboss {

BspTransceiverAccess::BspTransceiverAccess(
    uint32_t tcvr,
    BspTransceiverMapping& tcvrMapping) {
  tcvrMapping_ = tcvrMapping;
  tcvrID_ = tcvr;
  auto accessControlType = *tcvrMapping_.accessControl()->type();
  if (accessControlType == ResetAndPresenceAccessType::CPLD) {
    impl_ = std::make_unique<BspTransceiverCpldAccess>(tcvr, tcvrMapping);
// LibGpiod is not yet supported with OSS - See T126740581
#ifndef IS_OSS
  } else if (accessControlType == ResetAndPresenceAccessType::GPIO) {
    impl_ = std::make_unique<BspTransceiverGpioAccess>(tcvr, tcvrMapping);
#endif
  } else {
    throw BspTransceiverAccessError(
        "Invalid AccessControlType " +
        apache::thrift::util::enumNameSafe(accessControlType));
  }
}

void BspTransceiverAccess::init(bool forceReset) {
  impl_->init(forceReset);
}

bool BspTransceiverAccess::isPresent() {
  return impl_->isPresent();
}

void BspTransceiverAccess::holdReset() {
  impl_->holdReset();
}

void BspTransceiverAccess::releaseReset() {
  impl_->releaseReset();
}

} // namespace fboss
} // namespace facebook
