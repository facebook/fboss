// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/fsdb/if/FsdbModel.h>
#include <fboss/fsdb/oper/instantiations/FsdbCowRoot.h>
#include <fboss/thrift_cow/visitors/PathVisitor.h>

namespace facebook::fboss::thrift_cow {

extern template ThriftTraverseResult RootPathVisitor::visit<FsdbCowStateRoot>(
    FsdbCowStateRoot& node,
    pv_detail::PathIter begin,
    pv_detail::PathIter end,
    const PathVisitMode& mode,
    BasePathVisitorOperator& op);

extern template ThriftTraverseResult RootPathVisitor::visit<FsdbCowStatsRoot>(
    FsdbCowStatsRoot& node,
    pv_detail::PathIter begin,
    pv_detail::PathIter end,
    const PathVisitMode& mode,
    BasePathVisitorOperator& op);

} // namespace facebook::fboss::thrift_cow
