namespace cpp facebook.fboss
namespace cpp2 facebook.fboss
namespace d neteng.fboss.highres
namespace php fboss
namespace py neteng.fboss.highres

typedef string (cpp2.type = "::folly::fbstring") fbstring

enum SleepMethod {
  NANOSLEEP,
  PAUSE
}

struct CounterSubscribeRequest {
  /* What to subscribe to */
  // The set of all the counters to which the client wants to subscribe
  1 : set<string> counters,

  /* When to sample */
  // The maximum duration of the subscription in nanoseconds
  2 : i64 maxTime,
  // The maximum number of samples to take
  3 : i64 maxCount,
  // Interval between samples in nanoseconds
  4 : i64 intervalInNs,

  /* Performance tuning */
  // Number of samples to batch before publishing back to the client
  5 : i32 batchSize,
  // Whether to use nanosleep() or asm ("pause")
  6 : SleepMethod sleepMethod,
  // Whether to lower the priority of the sampling thread
  7 : bool veryNice
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
