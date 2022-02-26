// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/client/FsdbDeltaPublisher.h"

#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"

namespace facebook::fboss::fsdb {

void FsdbDeltaPublisher::write(OperDelta pubUnit) {
  if (!toPublishQueue_.try_enqueue(std::move(pubUnit))) {
    FsdbException ex;
    ex.errorCode_ref() = FsdbErrorCode::DROPPED;
    ex.message_ref() = "Unable to queue delta";
    throw ex;
  }
}
} // namespace facebook::fboss::fsdb
