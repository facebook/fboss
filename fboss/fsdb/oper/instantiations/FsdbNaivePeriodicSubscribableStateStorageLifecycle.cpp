/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <fboss/fsdb/oper/instantiations/FsdbNaivePeriodicSubscribableStorage.h>

namespace facebook::fboss::fsdb {

using StateStorage = FsdbNaivePeriodicSubscribableStorage;

template StateStorage::NaivePeriodicSubscribableStorage(
    const FsdbOperStateRoot&,
    NaivePeriodicSubscribableStorageBase::StorageParams);
template StateStorage::~NaivePeriodicSubscribableStorage();

} // namespace facebook::fboss::fsdb
