// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/instantiations/FsdbCowStateSubManager.h"
#include "fboss/fsdb/oper/instantiations/FsdbCowStorage.h"

namespace facebook::fboss::fsdb {

template class FsdbSubManager<CowStorage<FsdbOperStateRoot>, true /* IsCow */>;

} // namespace facebook::fboss::fsdb
