namespace py3 neteng.fboss
namespace py neteng.fboss.fsdb_common
namespace py.asyncio neteng.fboss.asyncio.fsdb_common
namespace cpp2 facebook.fboss.fsdb
namespace go facebook.fboss.fsdb_common

cpp_include "folly/container/F14Map.h"
cpp_include "folly/container/F14Set.h"

const string kFsdbServiceHandlerNativeStatsPrefix = "fsdb.handler.";
const string kFsdbStatsFanOutNativeStatsPrefix = "fsdb.statsFanOut.";

// NOTE: keep in sync with fb303::ExportType
enum ExportType {
  SUM = 0,
  COUNT = 1,
  AVG = 2,
  RATE = 3,
  PERCENT = 4,
}

typedef set<ExportType> (cpp.template = 'folly::F14FastSet') ExportTypes
typedef i64 Percentile
typedef set<Percentile> (cpp.template = 'std::set') Percentiles

struct AggregatorParamsTimeseries {
  1: ExportTypes exportTypes;
}

struct AggregatorParamsHistogram {
  1: i64 bucketWidth;
  2: i64 min;
  3: i64 max;
  4: ExportTypes exportTypes;
  5: Percentiles percentiles;
}

union AggregatorParams {
  1: AggregatorParamsTimeseries timeseries;
  2: AggregatorParamsHistogram histogram;
}

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
}

exception FsdbException {
  1: string message;
  2: FsdbErrorCode errorCode;
}

typedef string PublisherId
typedef string SubscriberId
typedef string Metric
typedef string ReMetric
typedef string EchoBackTag

typedef set<PublisherId> (cpp.template = 'folly::F14FastSet') PublisherIds
typedef set<SubscriberId> (cpp.template = 'folly::F14FastSet') SubscriberIds
typedef set<Metric> (cpp.template = 'folly::F14FastSet') Metrics
typedef map<ReMetric, EchoBackTag> (
  cpp.template = 'folly::F14FastMap',
) ReMetrics

// NOTE: Fully Qualified => Fq
struct FqMetric {
  1: PublisherId publisherId;
  2: Metric metric;
}

typedef map<PublisherId, Metrics> (cpp.template = 'folly::F14FastMap') FqMetrics

typedef string FqMetricAsKey
typedef string FqReMetricAsKey

typedef set<FqMetricAsKey> (cpp.template = 'folly::F14FastSet') FqMetricAsKeys

// NOTE: Regular Expression => Re
struct FqReMetric {
  1: PublisherId publisherId;
  2: ReMetric reMetric;
}

typedef map<PublisherId, ReMetrics> (
  cpp.template = 'folly::F14FastMap',
) FqReMetrics
