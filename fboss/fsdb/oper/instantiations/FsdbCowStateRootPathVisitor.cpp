// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fboss/fsdb/oper/instantiations/FsdbCowRootPathVisitor.h>

namespace facebook::fboss::thrift_cow {

template ThriftTraverseResult RootPathVisitor::visit<FsdbCowStateRoot>(
    FsdbCowStateRoot& node,
    pv_detail::PathIter begin,
    pv_detail::PathIter end,
    const PathVisitOptions& options,
    BasePathVisitorOperator& op);

} // namespace facebook::fboss::thrift_cow
