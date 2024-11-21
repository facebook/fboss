// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/common/Utils.h"
#include <folly/system/ThreadName.h>
#include <sstream>

namespace facebook::fboss::fsdb {

OperDelta createDelta(std::vector<OperDeltaUnit>&& deltaUnits) {
  OperDelta delta;
  delta.changes() = deltaUnits;
  delta.protocol() = OperProtocol::BINARY;
  return delta;
}

} // namespace facebook::fboss::fsdb
