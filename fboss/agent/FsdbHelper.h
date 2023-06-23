// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h>

#include <sstream>

namespace facebook::fboss {

std::vector<std::string> fsdbAgentDataSwitchStateRootPath();
std::vector<std::string> switchStateRootPath();

std::string getOperPath(const std::vector<std::string>& tokens);
void printOperDeltaPaths(const fsdb::OperDelta& operDelta);
} // namespace facebook::fboss
