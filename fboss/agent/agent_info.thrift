#
# Copyright 2004-present Facebook. All Rights Reserved.
#
include "thrift/annotation/thrift.thrift"

@thrift.AllowLegacyMissingUris
package;

namespace py neteng.fboss.agent_info
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.agent_info
namespace cpp2 facebook.fboss.agent_info
namespace go neteng.fboss.agent_info

struct AgentInfo {
  // timestamp in msec since epoch when wedge_agent started
  1: i64 startTime;
  // interval in msec for publishing stats to fsdb
  2: i64 fsdbStatsPublishIntervalMsec;
}
