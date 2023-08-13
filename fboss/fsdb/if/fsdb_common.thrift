namespace py3 neteng.fboss
namespace py neteng.fboss.fsdb_common
namespace py.asyncio neteng.fboss.asyncio.fsdb_common
namespace cpp2 facebook.fboss.fsdb
namespace go facebook.fboss.fsdb_common

cpp_include "folly/container/F14Map.h"
cpp_include "folly/container/F14Set.h"

include "thrift/annotation/cpp.thrift"

const string kFsdbServiceHandlerNativeStatsPrefix = "fsdb.handler.";
const string kFsdbStatsFanOutNativeStatsPrefix = "fsdb.statsFanOut.";

const i32 PORT = 5908;

// NOTE: keep in sync with fb303::ExportType
enum ExportType {
  SUM = 0,
  COUNT = 1,
  AVG = 2,
  RATE = 3,
  PERCENT = 4,
}

@cpp.Type{template = "folly::F14FastSet"}
typedef set<ExportType> ExportTypes

enum FsdbErrorCode {
  NONE = 0,
  ID_ALREADY_EXISTS = 1,
  ID_NOT_FOUND = 2,
  DROPPED = 3,
  EMPTY_SUBSCRIPTION_LIST = 4,
  NO_FAN_OUT = 5,
  INVALID_AGGREGATOR_PARAMS = 6,
  ROCKSDB_OPEN_OR_CREATE_FAILED = 7,
  UNKNOWN_PUBLISHER = 8,
  INVALID_PATH = 9,
  PUBLISHER_NOT_PERMITTED = 10,
  EMPTY_PUBLISHER_ID = 11,
  EMPTY_SUBSCRIBER_ID = 12,
  ALL_PUBLISHERS_GONE = 13,
  DISCONNECTED = 14,
  PUBLISHER_NOT_READY = 15,
}

exception FsdbException {
  1: string message;
  2: FsdbErrorCode errorCode;
}

typedef string PublisherId
typedef string SubscriberId
typedef string Metric
typedef string EchoBackTag

@cpp.Type{template = "folly::F14FastSet"}
typedef set<PublisherId> PublisherIds
@cpp.Type{template = "folly::F14FastSet"}
typedef set<SubscriberId> SubscriberIds
@cpp.Type{template = "folly::F14FastSet"}
typedef set<Metric> Metrics

// NOTE: Fully Qualified => Fq
struct FqMetric {
  1: PublisherId publisherId;
  2: Metric metric;
}

@cpp.Type{template = "folly::F14FastMap"}
typedef map<PublisherId, Metrics> FqMetrics

enum FsdbSubscriptionState {
  DISCONNECTED = 1,
  CONNECTED = 2,
}
