// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/FsdbAdaptedSubManagerTypes.h"
#include "fboss/fsdb/if/facebook/gen-cpp2/fsdb_model_types.h"
#include "fboss/thrift_cow/storage/CowStorage.h"

namespace facebook::fboss {

// Explicit template instantiation for CowStorage to improve compilation time
template class fsdb::CowStorage<fsdb::FsdbOperStateRoot, FsdbOperStateRoot>;

} // namespace facebook::fboss
