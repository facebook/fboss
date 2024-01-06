namespace cpp2 facebook.fboss
namespace go neteng.fboss.fboss_io_stats
namespace php io_stats
namespace py neteng.fboss.io_stats
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.io_stats

struct IOStats {
  // duration between last read and last successful read
  1: double readDownTime;
  // duration between last write and last successful write
  2: double writeDownTime;
  // Number of times the read transaction was attempted
  3: i64 numReadAttempted;
  // Number of times the read transaction failed
  4: i64 numReadFailed;
  // Number of times the write transaction was attempted
  5: i64 numWriteAttempted;
  // Number of times the write transaction failed
  6: i64 numWriteFailed;
}
