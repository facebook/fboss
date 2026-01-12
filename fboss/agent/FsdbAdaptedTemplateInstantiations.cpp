// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/FsdbAdaptedTemplateInstantiations.h"

namespace facebook::fboss {

// Explicit template instantiations for thrift_cow::deserialize (the costly ones
// used in DsfSubscription.cpp)
template MultiSwitchSystemPortMapThriftType thrift_cow::deserialize<
    MultiSwitchSystemPortMapTypeClass,
    MultiSwitchSystemPortMapThriftType>(
    fsdb::OperProtocol protocol,
    const folly::fbstring& encoded);

template MultiSwitchInterfaceMapThriftType thrift_cow::deserialize<
    MultiSwitchInterfaceMapTypeClass,
    MultiSwitchInterfaceMapThriftType>(
    fsdb::OperProtocol protocol,
    const folly::fbstring& encoded);

template fsdb::FsdbSubscriptionState thrift_cow::deserialize<
    apache::thrift::type_class::enumeration,
    fsdb::FsdbSubscriptionState>(
    fsdb::OperProtocol protocol,
    const folly::fbstring& encoded);

} // namespace facebook::fboss
