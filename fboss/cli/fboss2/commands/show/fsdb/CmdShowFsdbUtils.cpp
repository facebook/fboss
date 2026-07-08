// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/show/fsdb/CmdShowFsdbUtils.h"

#include <iostream>

namespace facebook::fboss {

void dumpFsdbMetadata(const facebook::fboss::fsdb::OperMetadata& meta) {
  // lastConfirmedAt: seconds since epoch (per fsdb_oper.thrift:54).
  // lastPublishedAt / lastServedAt: msec since epoch (per fsdb_oper.thrift:56,
  // 58). Dump metadata in stderr so that stdout remains JSON-parsable.
  // Uses fsdb_cli_format helpers from CmdShowFsdbUtils.h (std::put_time
  // based, so no asctime_r NULL-return risk).
  auto lastConfirmedAt = meta.lastConfirmedAt().value_or(0);
  auto lastPublishedAtMs = meta.lastPublishedAt().value_or(0);
  auto lastServedAtMs = meta.lastServedAt().value_or(0);

  std::cerr << fmt::format(
      "Last confirmed at: {} -- {}\n",
      lastConfirmedAt,
      fsdb_cli_format::epochSecondsAsLocalTime(lastConfirmedAt));
  std::cerr << fmt::format(
      "Last published at: {} -- {}\n",
      lastPublishedAtMs,
      fsdb_cli_format::epochMillisAsLocalTime(lastPublishedAtMs));
  std::cerr << fmt::format(
      "Last served at:    {} -- {}\n",
      lastServedAtMs,
      fsdb_cli_format::epochMillisAsLocalTime(lastServedAtMs));
}

} // namespace facebook::fboss
