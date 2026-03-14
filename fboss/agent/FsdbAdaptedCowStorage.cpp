// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/FsdbAdaptedCowStorage.h"

namespace facebook::fboss {

// Explicit template instantiation for CowStorage to improve compilation time
template class fsdb::CowStorage<fsdb::FsdbOperStateRoot, FsdbOperStateRoot>;

} // namespace facebook::fboss
