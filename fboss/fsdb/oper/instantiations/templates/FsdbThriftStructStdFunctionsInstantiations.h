/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <fboss/thrift_cow/nodes/ThriftStructNode-inl.h>
#include <memory>
#include "fboss/fsdb/if/FsdbModel.h"

namespace std {

// Extern templates for std::make_shared and related functions
extern template shared_ptr<facebook::fboss::thrift_cow::ThriftStructNode<
    facebook::fboss::fsdb::FsdbOperStateRoot>>
make_shared<
    facebook::fboss::thrift_cow::ThriftStructNode<
        facebook::fboss::fsdb::FsdbOperStateRoot>,
    const facebook::fboss::fsdb::FsdbOperStateRoot&>(
    const facebook::fboss::fsdb::FsdbOperStateRoot&);

extern template shared_ptr<facebook::fboss::thrift_cow::ThriftStructNode<
    facebook::fboss::fsdb::FsdbOperStateRoot>>
allocate_shared<
    facebook::fboss::thrift_cow::ThriftStructNode<
        facebook::fboss::fsdb::FsdbOperStateRoot>,
    allocator<facebook::fboss::thrift_cow::ThriftStructNode<
        facebook::fboss::fsdb::FsdbOperStateRoot>>,
    const facebook::fboss::fsdb::FsdbOperStateRoot&>(
    const allocator<facebook::fboss::thrift_cow::ThriftStructNode<
        facebook::fboss::fsdb::FsdbOperStateRoot>>&,
    const facebook::fboss::fsdb::FsdbOperStateRoot&);

// Extern templates for AgentData
extern template shared_ptr<facebook::fboss::thrift_cow::ThriftStructNode<
    facebook::fboss::fsdb::AgentData>>
make_shared<
    facebook::fboss::thrift_cow::ThriftStructNode<
        facebook::fboss::fsdb::AgentData>,
    const facebook::fboss::fsdb::AgentData&>(
    const facebook::fboss::fsdb::AgentData&);

extern template shared_ptr<facebook::fboss::thrift_cow::ThriftStructNode<
    facebook::fboss::fsdb::AgentData>>
allocate_shared<
    facebook::fboss::thrift_cow::ThriftStructNode<
        facebook::fboss::fsdb::AgentData>,
    allocator<facebook::fboss::thrift_cow::ThriftStructNode<
        facebook::fboss::fsdb::AgentData>>,
    const facebook::fboss::fsdb::AgentData&>(
    const allocator<facebook::fboss::thrift_cow::ThriftStructNode<
        facebook::fboss::fsdb::AgentData>>&,
    const facebook::fboss::fsdb::AgentData&);

} // namespace std
