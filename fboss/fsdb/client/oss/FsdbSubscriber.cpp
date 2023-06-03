// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/FsdbSubscriber.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

namespace facebook::fboss::fsdb {

template <typename SubUnit, typename PathElement>
void FsdbSubscriber<SubUnit, PathElement>::checkServerState() {}

template class FsdbSubscriber<OperDelta, std::string>;
template class FsdbSubscriber<OperState, std::string>;
template class FsdbSubscriber<OperSubPathUnit, ExtendedOperPath>;
template class FsdbSubscriber<OperSubDeltaUnit, ExtendedOperPath>;
} // namespace facebook::fboss::fsdb
