// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/server/RocksDb.h"
#include "fboss/facebook/fsdb/utils/Utils.h"
#include "fboss/fsdb/common/Utils.h"

// TODO: use a better default dir
DEFINE_string(
    rocksdbDir,
    "/tmp/fsdb_rocksDB/",
    "Directory to contain rocksdb-s for stats and oper state");

namespace facebook::fboss::fsdb {

RocksDb::RocksDb(
    const std::string& category,
    const PublisherId& publisherId,
    const std::chrono::seconds ttl,
    bool eraseExisting)
    : publisherId_(publisherId),
      path_(folly::to<std::string>(
          FLAGS_rocksdbDir,
          category,
          "/",
          publisherId,
          "-db")),
      ttl_(ttl) {
  XLOG(INFO) << "RocksDb ctor for " << path_;
  if (eraseExisting) {
    XLOG(INFO) << "delete existing rocksdb at " << path_;
    std::filesystem::remove_all(path_);
  }
}

RocksDb::~RocksDb() {
  if (dbWithTTL_) { // close() was not called by application
    if (close()) {
      XLOG(INFO) << "closed rocksdb at " << path_;
    } else {
      XLOG(WARN) << "could not close rocksdb at " << path_;
    }
  }
}

bool RocksDb::open() {
  XLOG(INFO) << "create/open rocksdb for " << path_;

  std::error_code ec;
  std::filesystem::create_directories(path_, ec);
  if (ec) {
    XLOG(ERR) << "ec = " << ec.message();
    return false;
  }

  rocksdb::Options options;
  options.create_if_missing = true;

  try {
    auto status =
        rocksdb::DBWithTTL::Open(options, path_, &dbWithTTL_, ttl_.count());
    if (!status.ok()) {
      XLOG(ERR) << "could not open or create rocksdb: " << path_ << " - "
                << status.ToString();
      return false;
    }
  } catch (const std::exception& ex) {
    XLOG(ERR) << "caught exception on rocksdb open: " << path_ << " - "
              << ex.what();
  }

  return true;
}

bool RocksDb::close(bool erase) {
  XLOG(INFO) << "close rocksdb for " << path_;
  CHECK(dbWithTTL_);

  bool ret = true;
  {
    auto status = dbWithTTL_->SyncWAL();
    if (!status.ok()) {
      XLOG(ERR) << "Error on sync WAL for db for " << path_ << " : "
                << status.ToString();
      ret = ret && false;
    }
  }

  {
    auto status = dbWithTTL_->Close();
    if (!status.ok()) {
      XLOG(ERR) << "Error on closing db for " << path_ << " : "
                << status.ToString();
      ret = ret && false;
    }
  }

  delete dbWithTTL_;
  dbWithTTL_ = nullptr;

  if (erase) {
    if (ret) {
      XLOG(INFO) << "delete rocksdb at " << path_ << " on close";
      std::filesystem::remove_all(path_);
    } else {
      XLOG(INFO) << "skip delete of rocksdb at " << path_
                 << " on close due to previous failure";
    }
  }

  return ret;
}

bool RocksDb::get(const std::string& key, std::string* value) {
  XLOG(DBG3) << "get for key = " << key;
  CHECK(dbWithTTL_);
  auto status = dbWithTTL_->Get(rocksdb::ReadOptions(), key, value);
  if (!status.ok()) {
    XLOG(ERR) << "Error on db get for " << path_ << " : " << status.ToString();
    return false;
  }
  return true;
}

bool RocksDb::put(const std::string& key, const std::string& value) {
  XLOG(DBG3) << "put for key = " << key << " value.size() = " << value.size();
  CHECK(dbWithTTL_);
  auto status = dbWithTTL_->Put(rocksdb::WriteOptions(), key, value);
  if (!status.ok()) {
    XLOG(ERR) << "Error on db put for " << path_ << " : " << status.ToString();
    return false;
  }
  return true;
}

std::vector<StatsSnapshotPtr> RocksDb::getSnapshots(
    const Metric& metric,
    int64_t fromTimestampSec) {
  XLOG(INFO) << "get historical snapshots for metric = " << metric
             << ", fromTimestampSec = " << fromTimestampSec;
  auto it = dbWithTTL_->NewIterator(rocksdb::ReadOptions());
  it->Seek(folly::to<std::string>(
      metric, ".", Utils::stringifyTimestampSec(fromTimestampSec)));

  std::vector<StatsSnapshotPtr> ret;
  for (; it->Valid(); it->Next()) {
    const auto [readMetric, readTimestampSec] =
        Utils::decomposeKey(it->key().ToString());
    if (metric != readMetric) {
      break;
    }

    ret.push_back(std::make_shared<StatsSnapshot>(
        Utils::getFqMetric(publisherId_, readMetric),
        readTimestampSec,
        it->value().ToString()));
  }

  it->Reset();
  delete it;
  return ret;
}

// TODO: is there a better way to count than seek and iterate till end ?
int64_t RocksDb::getNumSnapshots(const Metric& metric, int64_t fromTimestampSec)
    const {
  XLOG(INFO) << "get number of historical snapshots for metric = " << metric
             << ", fromTimestampSec = " << fromTimestampSec;
  // if fromTimestampSec is specified, metric cannot be unspecified
  CHECK(metric != "" || !fromTimestampSec);

  auto it = dbWithTTL_->NewIterator(rocksdb::ReadOptions());
  const auto seekTo = (metric != "")
      ? folly::to<std::string>(
            metric, ".", Utils::stringifyTimestampSec(fromTimestampSec))
      : "";
  it->Seek(seekTo);

  int64_t ret = 0;
  for (; it->Valid(); it->Next()) {
    ++ret;
  }

  it->Reset();
  delete it;
  return ret;
}

} // namespace facebook::fboss::fsdb
