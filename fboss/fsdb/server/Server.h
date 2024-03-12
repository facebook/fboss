// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/server/FsdbConfig.h"
#include "fboss/fsdb/server/ServiceHandler.h"

namespace facebook::fboss::fsdb {

std::shared_ptr<ServiceHandler> createThriftHandler(
    std::shared_ptr<FsdbConfig> fsdbConfig);

std::shared_ptr<apache::thrift::ThriftServer> createThriftServer(
    std::shared_ptr<FsdbConfig> fsdbConfig,
    std::shared_ptr<ServiceHandler> handler);

void startThriftServer(
    std::shared_ptr<apache::thrift::ThriftServer> server,
    std::shared_ptr<ServiceHandler> handler);

std::shared_ptr<FsdbConfig> parseConfig(int argc, char** argv);

void initFlagDefaults(
    const std::unordered_map<std::string, std::string>& defaults);

} // namespace facebook::fboss::fsdb
