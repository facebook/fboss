// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/client/FsdbDeltaSubscriber.h"

namespace facebook::fboss::fsdb {

template <typename SubUnit>
OperSubRequest FsdbSubscriber<SubUnit>::createRequest() const {
  OperPath operPath;
  operPath.raw_ref() = subscribePath_;
  OperSubRequest request;
  request.path_ref() = operPath;
  return request;
}

template class FsdbSubscriber<OperDelta>;
template class FsdbSubscriber<OperState>;

} // namespace facebook::fboss::fsdb
