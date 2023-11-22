/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/fsdb/oper/instantiations/FsdbNaivePeriodicSubscribableStorage.h"

#include <folly/experimental/coro/AsyncScope.h>
#include <folly/experimental/coro/Task.h>
#include <rocksdb/db.h>
#include <thread>

namespace facebook::fboss::fsdb {

class DbWriter {
 public:
  explicit DbWriter(FsdbNaivePeriodicSubscribableStorage& storage);
  ~DbWriter();

  void start();
  void stop();

 private:
  folly::coro::Task<void> subscribeAndWrite();
  void snapshotFullState(std::optional<std::string> key = std::nullopt);
  void writeDelta(OperDelta&& delta);
  std::optional<OperState> readFullSnapshot(const std::string& key);

  // Forbidden copy constructor and assignment operator
  DbWriter(DbWriter const&) = delete;
  DbWriter& operator=(DbWriter const&) = delete;

  std::atomic_bool running_{false};

  std::unique_ptr<std::thread> writeThread_;
  folly::EventBase evb_;
  std::unique_ptr<rocksdb::DB> db_{nullptr};

  // we require caller to ensure that the storage must outlive this
  // class
  FsdbNaivePeriodicSubscribableStorage& storage_;

  folly::coro::CancellableAsyncScope backgroundScope_;

  uint64_t lastFullSnapshotGeneration_{0};
  uint64_t numDeltasSinceFullSnapshot_{0};
  uint64_t runId_{0};
};

} // namespace facebook::fboss::fsdb
