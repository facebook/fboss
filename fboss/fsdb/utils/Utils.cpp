// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/utils/Utils.h"
#include <folly/String.h>
#include <folly/system/ThreadName.h>

namespace facebook::fboss::fsdb {

void initThread(folly::StringPiece name) {
  folly::setThreadName(name);
  google::SetLogThreadName(name.str());
}

} // namespace facebook::fboss::fsdb
