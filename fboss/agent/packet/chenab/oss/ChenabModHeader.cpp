// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "fboss/agent/packet/chenab/ChenabModHeader.h"

#include "fboss/agent/packet/HdrParseError.h"

namespace facebook::fboss {

// NOLINTNEXTLINE(clang-diagnostic-missing-noreturn)
ChenabTacEvent parseChenabTacEvent(folly::io::Cursor&) {
  throw HdrParseError("Mirror-on-drop packet decoding is unavailable in OSS");
}

} // namespace facebook::fboss
