#
# Copyright 2004-present Facebook. All Rights Reserved.
#
namespace py neteng.fboss.cable_info
namespace cpp2 facebook.fboss.cfg

struct CableInfo {
  1: double cableLength,
  2: i32 cableGauge,
}

struct FbDACTable{
  1: map<string, CableInfo> pnToCableInfoMap,
}
