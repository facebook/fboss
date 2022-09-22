// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/util/qsfp/QsfpServiceDetector.h"
#include <folly/Singleton.h>
#include <folly/logging/xlog.h>
#include "fboss/qsfp_service/lib/QsfpClient.h"

namespace facebook::fboss {

folly::Singleton<QsfpServiceDetector> _qsfpServiceDetector;

std::shared_ptr<QsfpServiceDetector> QsfpServiceDetector::getInstance() {
  return _qsfpServiceDetector.try_get();
}

QsfpServiceDetector::QsfpServiceDetector() {
  detectQsfpService();
}

/*
 * This function detects whether or not qsfp_service is running.
 */
void QsfpServiceDetector::detectQsfpService() {
  try {
    auto qsfpServiceClient = utils::createQsfpServiceClient();

    if (qsfpServiceClient &&
        facebook::fb303::cpp2::fb_status::ALIVE ==
            qsfpServiceClient.get()->sync_getStatus()) {
      qsfpServiceActive_ = true;
    }

    if (qsfpServiceActive_) {
      printf("qsfp_service is running\n");
    } else {
      printf("qsfp_service is not running\n");
    }
  } catch (const std::exception& ex) {
    XLOG(DBG5) << fmt::format(
        "Exception connecting to qsfp_service: {}", folly::exceptionStr(ex));
  }
}

} // namespace facebook::fboss
