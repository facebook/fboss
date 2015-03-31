namespace cpp facebook.fboss
namespace cpp2 facebook.fboss
namespace py neteng.fboss.highres
namespace d neteng.fboss.highres

typedef string (cpp2.type = "::folly::fbstring") fbstring

struct CounterSubscribeRequest {
  // The set of all the counters to which the client wants to subscribe
  1 : set<string> counters,
  // The maximum duration of the subscription in nanoseconds
  2 : i64 maxTime,
  // The maximum number of samples to take
  3 : i64 maxCount,
  // Interval between samples in nanoseconds
  4 : i64 intervalInNs,
  // Number of samples to batch before publishing back to the client
  5 : i32 batchSize
}

struct HighresTime {
  1: i64 seconds,
  2: i64 nanoseconds
}

struct CounterPublication {
  // Full hostname of the publishing server
  1: string hostname,
  // A list of sample times included in this batch
  2: list<HighresTime> times,
  // All of the samples for this batch:
  // {namespace::counter name} -> {list of sample values}
  // The list corresponds to the times in highres_time
  3: map<string,list<i64>> counters
}

service FbossHighresClient {
  oneway void publishCounters(1: CounterPublication pub) (thread='eb')
}
