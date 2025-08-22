/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/fsdb/oper/instantiations/templates/FsdbPathVisitorOperStateInstantiations.h"

namespace facebook::fboss::thrift_cow {

// Explicit instantiation for PathVisitor::visit with const node and
// BasePathVisitorOperator
template ThriftTraverseResult
PathVisitor<apache::thrift::type_class::structure>::visit(
    const ThriftStructNode<fsdb::FsdbOperStateRoot>& node,
    pv_detail::PathIter begin,
    pv_detail::PathIter end,
    const PathVisitOptions& options,
    BasePathVisitorOperator& op);

// Explicit instantiation for PathVisitorImpl::visit with const node and
// BasePathVisitorOperator
template ThriftTraverseResult
pv_detail::PathVisitorImpl<apache::thrift::type_class::structure>::visit(
    const ThriftStructNode<fsdb::FsdbOperStateRoot>& node,
    const pv_detail::VisitImplParams<BasePathVisitorOperator>& params,
    pv_detail::PathIter cursor);

// Explicit instantiation for visitNode with const node and
// BasePathVisitorOperator
template ThriftTraverseResult pv_detail::visitNode<
    apache::thrift::type_class::structure,
    const ThriftStructNode<fsdb::FsdbOperStateRoot>,
    BasePathVisitorOperator>(
    const ThriftStructNode<fsdb::FsdbOperStateRoot>& node,
    const pv_detail::VisitImplParams<BasePathVisitorOperator>& params,
    pv_detail::PathIter cursor);

// Explicit instantiation for PathVisitorImpl::visit with const
// ThriftStructFields and BasePathVisitorOperator
template ThriftTraverseResult
pv_detail::PathVisitorImpl<apache::thrift::type_class::structure>::visit(
    const ThriftStructFields<
        fsdb::FsdbOperStateRoot,
        ThriftStructNode<fsdb::FsdbOperStateRoot>>& fields,
    const pv_detail::VisitImplParams<BasePathVisitorOperator>& params,
    pv_detail::PathIter cursor);

} // namespace facebook::fboss::thrift_cow
