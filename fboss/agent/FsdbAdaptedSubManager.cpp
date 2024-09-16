// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/FsdbAdaptedSubManager.h"

namespace facebook::fboss {

template class fsdb::FsdbSubManager<
    fsdb::CowStorage<fsdb::FsdbOperStateRoot, FsdbOperStateRoot>,
    true /* IsCow */>;

} // namespace facebook::fboss
