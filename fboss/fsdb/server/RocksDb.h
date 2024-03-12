// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <folly/Utility.h>
#include <folly/logging/xlog.h>
#include <filesystem>
#include "fboss/fsdb/if/FsdbModel.h"
#include "rocksdb/utilities/db_ttl.h"

DECLARE_string(rocksdbDir);

namespace facebook::fboss::fsdb {

class RocksDbIf : public folly::MoveOnly {
 public:
  RocksDbIf() = default;
  virtual ~RocksDbIf() = default;

  virtual bool open() = 0;
  virtual bool close(bool erase = false) = 0;

  virtual bool get(const std::string& key, std::string* value) = 0;
  virtual bool put(const std::string& key, const std::string& value) = 0;

  virtual int64_t getNumSnapshots(
      const Metric& metric = "",
      int64_t fromTimestampSec = 0) const = 0;
};

class RocksDb : public RocksDbIf {
 public:
  RocksDb(
      const std::string& category,
      const PublisherId& publisherId,
      const std::chrono::seconds ttl = {},
      bool eraseExisting = false);
  ~RocksDb() override;

  bool open() override;
  bool close(bool erase = false) override;

  bool get(const std::string& key, std::string* value) override;
  bool put(const std::string& key, const std::string& value) override;

  int64_t getNumSnapshots(
      const Metric& metric = "",
      int64_t fromTimestampSec = 0) const override;

 private:
  const PublisherId publisherId_;
  const std::filesystem::path path_;
  const std::chrono::seconds ttl_;
  rocksdb::DBWithTTL* dbWithTTL_{nullptr};
};

// Used as a sink in tests, asserts if read from
class RocksDbFake : public RocksDbIf {
 public:
  RocksDbFake(
      const std::string& /* category */,
      const PublisherId& /* publisherId */,
      const std::chrono::seconds /* ttl */ = {},
      bool /* eraseExisting */ = false) {}

  bool open() override {
    return true;
  }
  bool close(bool /* erase */ = false) override {
    return true;
  }

  bool get(const std::string& /* key */, std::string* /* value */) override {
    XLOG(FATAL) << "Fake rocksdb for tests cannot be read from (yet)";
    return false;
  }
  bool put(const std::string& /* key */, const std::string& /* value */)
      override {
    return true;
  }

  int64_t getNumSnapshots(
      const Metric& /* metric */ = "",
      int64_t /* fromTimestampSec */ = 0) const override {
    XLOG(FATAL) << "Fake rocksdb for tests cannot be read from (yet)";
    return 0;
  }
};

} // namespace facebook::fboss::fsdb
