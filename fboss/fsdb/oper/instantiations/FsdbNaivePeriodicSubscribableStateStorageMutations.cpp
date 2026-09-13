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

template std::optional<StorageError> StateStorage::set_encoded_impl(
    StateStorage::PathIter,
    StateStorage::PathIter,
    const OperState&);
template void StateStorage::remove_impl(
    StateStorage::PathIter,
    StateStorage::PathIter);
template std::optional<StorageError> StateStorage::patch_impl(Patch&&);
template std::optional<StorageError> StateStorage::patch_impl(const OperDelta&);
template std::optional<StorageError> StateStorage::patch_impl(
    const TaggedOperState&);

} // namespace facebook::fboss::fsdb
