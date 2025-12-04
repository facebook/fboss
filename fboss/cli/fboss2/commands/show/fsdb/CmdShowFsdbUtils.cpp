// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/show/fsdb/CmdShowFsdbUtils.h"

#include <ctime>
#include <iostream>

namespace facebook::fboss {

void dumpFsdbMetadata(const facebook::fboss::fsdb::OperMetadata& meta) {
  auto lastConfirmedAt = meta.lastConfirmedAt().value_or(0);
  std::tm tm;
  localtime_r(&lastConfirmedAt, &tm);
  std::array<char, 26> buf;
  auto formattedTime = asctime_r(&tm, buf.data());
  // Dump metadata in stderr to that stdout is json parsable
  std::cerr << fmt::format(
      "Last confirmed at: {} -- {}", lastConfirmedAt, formattedTime);
}

} // namespace facebook::fboss
