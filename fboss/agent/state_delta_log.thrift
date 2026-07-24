// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

package "facebook.com/fboss/agent/state_delta_log"

namespace cpp2 facebook.fboss

include "fboss/fsdb/if/fsdb_oper.thrift"

struct StateDeltaLogRecord {
  1: i64 timestampMs;
  2: string reason;
  3: i64 oldGeneration;
  4: i64 newGeneration;
  5: binary operDeltaBytes;
  6: fsdb_oper.OperProtocol operProtocol;
}

struct BootHeader {
  1: i64 timestampMs;
  2: string commitId;
  3: bool warmBoot;
  4: string sdkVersion;
}

// Unpaired CHECKPOINT[seq] in the log indicates a lost delta at that seq.
struct CheckpointRecord {
  1: i64 seqNum;
  2: i64 timestampMs;
  3: i64 oldGeneration;
  4: i64 newGeneration;
}

struct MalformedDeltaRecord {
  1: i64 seqNum;
  2: i64 timestampMs;
  3: string exceptionMessage;
}
