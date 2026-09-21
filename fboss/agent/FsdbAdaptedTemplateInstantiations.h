// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/SystemPortMap.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/thrift_cow/nodes/Serializer.h"

#include <folly/FBString.h>

namespace facebook::fboss {

// Extern template declarations for thrift_cow::deserialize (the costly ones
// used in DsfSubscription.cpp)
extern template MultiSwitchSystemPortMapThriftType thrift_cow::deserialize<
    MultiSwitchSystemPortMapTypeClass,
    MultiSwitchSystemPortMapThriftType>(
    fsdb::OperProtocol protocol,
    const folly::fbstring& encoded);

extern template MultiSwitchInterfaceMapThriftType thrift_cow::deserialize<
    MultiSwitchInterfaceMapTypeClass,
    MultiSwitchInterfaceMapThriftType>(
    fsdb::OperProtocol protocol,
    const folly::fbstring& encoded);

extern template fsdb::FsdbSubscriptionState thrift_cow::deserialize<
    apache::thrift::type_class::enumeration,
    fsdb::FsdbSubscriptionState>(
    fsdb::OperProtocol protocol,
    const folly::fbstring& encoded);
} // namespace facebook::fboss
