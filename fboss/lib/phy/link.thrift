#
# Copyright 2004-present Facebook. All Rights Reserved.
#
namespace py neteng.fboss.link
namespace py3 neteng.fboss.link
namespace py.asyncio neteng.fboss.asyncio.link
namespace cpp2 facebook.fboss.link
namespace go neteng.fboss.link
namespace php fboss_link

struct LinkPerfMonitorParamEachSideVal {
  1: double min;
  2: double max;
  3: double avg;
  4: double cur;
}
