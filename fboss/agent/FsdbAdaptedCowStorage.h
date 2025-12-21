// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/FsdbAdaptedSubManagerTypes.h"
#include "fboss/thrift_cow/storage/CowStorage.h"

namespace facebook::fboss {

// Extern template declaration to prevent local instantiation
// The actual instantiation is in FsdbAdaptedCowStorage.cpp

// CowStorage extern template declaration
extern template class fsdb::
    CowStorage<fsdb::FsdbOperStateRoot, FsdbOperStateRoot>;

} // namespace facebook::fboss
