// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include <string>

#include "fboss/fsdb/common/Utils.h"
#include "fboss/fsdb/if/FsdbModel.h"

namespace facebook::fboss {

// This templates is expensive and compiled here to parallelize the build
template fsdb::OperDelta
fsdb::computeOperDelta<thrift_cow::ThriftStructNode<fsdb::AgentData>>(
    const std::shared_ptr<thrift_cow::ThriftStructNode<fsdb::AgentData>>&,
    const std::shared_ptr<thrift_cow::ThriftStructNode<fsdb::AgentData>>&,
    const std::vector<std::string>&,
    bool);

} // namespace facebook::fboss
