// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/thrift_cow/gen-cpp2/patch_types.h>
#include <fboss/thrift_cow/visitors/DeltaVisitor.h>
#include <fboss/thrift_cow/visitors/TraverseHelper.h>

#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <functional>

namespace facebook::fboss::thrift_cow {

namespace detail_pb {
struct PatchBuilderTraverser : public TraverseHelper<PatchBuilderTraverser> {
  using Base = TraverseHelper<PatchBuilderTraverser>;

  explicit PatchBuilderTraverser(PatchNode& rootPatch) {
    curPath_ = {rootPatch};
  }

  bool shouldShortCircuitImpl(VisitorType visitorType) const {
    return false;
  }

  template <ThriftSimpleTC SimpleTC>
  void onPushImpl() {
    const auto& newTok = path().back();
    PatchNode& curPatch = curPath_.back();

    // TODO: use correct type and key
    PatchNode nextPatch;
    nextPatch.set_struct_node();
    auto it = curPatch.mutable_struct_node().children()->insert_or_assign(
        1, std::move(nextPatch));
    curPath_.push_back(it.first->second);
  }

  template <ThriftSimpleTC SimpleTC>
  void onPopImpl() {
    // TODO: prune empty paths on the way up
    curPath_.pop_back();
  }

 private:
  std::vector<std::reference_wrapper<PatchNode>> curPath_;
};
} // namespace detail_pb

struct PatchBuilder {
  template <typename Node>
  static thrift_cow::Patch build(
      const std::shared_ptr<Node>& oldNode,
      const std::shared_ptr<Node>& newNode,
      const std::vector<std::string>& basePath) {
    thrift_cow::Patch patch;
    patch.basePath() = basePath;

    auto processDelta = [](const std::vector<std::string>& path,
                           auto oldNode,
                           auto newNode,
                           thrift_cow::DeltaElemTag visitTag) {
      // TODO: patch node
    };

    patch.patch()->set_struct_node();
    detail_pb::PatchBuilderTraverser traverser(*patch.patch());

    thrift_cow::RootDeltaVisitor::visit(
        traverser,
        oldNode,
        newNode,
        thrift_cow::DeltaVisitOptions(
            thrift_cow::DeltaVisitMode::MINIMAL,
            DeltaVisitOrder::PARENTS_FIRST,
            true),
        std::move(processDelta));
    return patch;
  }
};

} // namespace facebook::fboss::thrift_cow
