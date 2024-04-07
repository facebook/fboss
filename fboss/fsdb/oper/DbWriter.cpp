/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/fsdb/oper/DbWriter.h"
#include "fboss/fsdb/server/RocksDb.h"

#include <folly/Conv.h>
#include <folly/experimental/coro/BlockingWait.h>
#include <folly/system/ThreadName.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <chrono>
#include <filesystem>

namespace {

std::string rocksDir() {
  return folly::to<std::string>(FLAGS_rocksdbDir, "oper/");
}

std::string rocksPath() {
  return folly::to<std::string>(rocksDir(), "db");
}

uint64_t genRunId() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

// probably want BINARY, but preferring readability for now...
constexpr auto kProtocol = facebook::fboss::fsdb::OperProtocol::SIMPLE_JSON;
constexpr auto kMaxDeltasBetweenFullSync = 500;
constexpr auto kPreShutdownKey = "pre-shutdown-state";

} // namespace

namespace facebook::fboss::fsdb {

DbWriter::DbWriter(FsdbNaivePeriodicSubscribableStorage& storage)
    : storage_(storage), runId_(genRunId()) {}

DbWriter::~DbWriter() {
  XLOG(INFO) << "destroy DbWriter";
  stop();
}

void DbWriter::start() {
  if (running_) {
    // already started
    return;
  }

  writeThread_ = std::make_unique<std::thread>([this]() {
    folly::setThreadName("DbWriterThread");
    evb_.loopForever();
  });

  rocksdb::Options options;
  options.create_if_missing = true;

  std::error_code ec;
  std::filesystem::create_directories(std::filesystem::path(rocksDir()), ec);
  if (ec) {
    XLOG(ERR) << "ec = " << ec.message();
    return;
  }

  rocksdb::DB* db;
  auto status = rocksdb::DB::Open(options, rocksPath(), &db);
  if (!status.ok()) {
    throw std::runtime_error(folly::to<std::string>(
        "Failed to open oper db for ",
        rocksPath(),
        ". msg=",
        status.ToString()));
  }

  db_.reset(db);

  backgroundScope_.add(subscribeAndWrite().scheduleOn(&evb_));

  running_ = true;
}

void DbWriter::stop() {
  if (!running_) {
    // never started
    return;
  }

  XLOG(INFO) << "DbWriter: Stopping...";

  folly::coro::blockingWait(backgroundScope_.cancelAndJoinAsync());

  XLOG(INFO) << "DbWriter: cancelled...";

  evb_.runImmediatelyOrRunInEventBaseThreadAndWait([this] {
    if (!db_) {
      snapshotFullState(kPreShutdownKey);

      auto status = db_->SyncWAL();
      if (!status.ok()) {
        XLOG(ERR) << "Error on sync WAL for oper db: " << status.ToString();
      }

      status = db_->Close();
      if (!status.ok()) {
        XLOG(ERR) << "Error closing oper db: " << status.ToString();
      }

      db_.reset();
    }
    evb_.terminateLoopSoon();
  });

  writeThread_->join();
  XLOG(INFO) << "DbWriter: joined...";

  running_ = false;
}

folly::coro::Task<void> DbWriter::subscribeAndWrite() {
  std::vector<std::string> rootPath;

  auto gen = storage_.subscribe_delta(
      "DbWriter", rootPath.begin(), rootPath.end(), kProtocol);
  while (auto item = co_await gen.next()) {
    XLOG(DBG3) << "DbWriter: item received";

    writeDelta(*item);
    if (++numDeltasSinceFullSnapshot_ > kMaxDeltasBetweenFullSync ||
        lastFullSnapshotGeneration_ == 0) {
      snapshotFullState();
      numDeltasSinceFullSnapshot_ = 0;
    }
  }

  XLOG(INFO) << "DbWriter: exiting subscribeAndWrite()";
  co_return;
}

void DbWriter::snapshotFullState(std::optional<std::string> key) {
  if (!db_) {
    return;
  }

  auto fullState = storage_.publishedStateEncoded(kProtocol);
  if (auto metadata = fullState.metadata()) {
    if (!key) {
      key = fmt::format("{}-s", runId_);
    }
    auto value = thrift_cow::serialize<apache::thrift::type_class::structure>(
                     kProtocol, fullState)
                     .toStdString();

    auto status = db_->Put(rocksdb::WriteOptions(), *key, value);
    if (!status.ok()) {
      throw std::runtime_error(folly::to<std::string>(
          "Error on db put for oper db: ", status.ToString()));
    } else {
      lastFullSnapshotGeneration_ = metadata->lastConfirmedAt().value_or(0);
    }
  }
}

void DbWriter::writeDelta(OperDelta&& delta) {
  if (!db_) {
    return;
  }

  if (auto metadata = delta.metadata()) {
    // we rely on the internal lastConfirmedAt for ordering our
    // deltas and synchronizing the streamed deltas with full state
    // snapshots.
    auto lastConfirmedAt = metadata->lastConfirmedAt().value_or(0);
    auto key = fmt::format("{}-d{}", runId_, lastConfirmedAt);
    auto value = thrift_cow::serialize<apache::thrift::type_class::structure>(
                     kProtocol, delta)
                     .toStdString();

    XLOG(DBG1) << "DbWriter: saving delta for lastConfirmedAt="
               << lastConfirmedAt;
    XLOG(DBG4) << "DbWriter: writing " << key << "=" << value;
    auto status = db_->Put(rocksdb::WriteOptions(), key, value);
    if (!status.ok()) {
      XLOG(ERR) << "Error on db put for oper db: " << status.ToString();
    }
  }
}

std::optional<OperState> DbWriter::readFullSnapshot(const std::string& key) {
  if (!db_) {
    return std::nullopt;
  }

  std::string value;
  auto status = db_->Get(rocksdb::ReadOptions(), key, &value);
  if (!status.ok()) {
    XLOG(ERR) << "Error on db get for " << key << " : " << status.ToString();
    return std::nullopt;
  }

  return thrift_cow::
      deserialize<apache::thrift::type_class::structure, OperState>(
          kProtocol, value);
}

} // namespace facebook::fboss::fsdb
