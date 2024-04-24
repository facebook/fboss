// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h>
#include <fboss/thrift_cow/gen-cpp2/patch_types.h>
#include <fboss/thrift_cow/gen-cpp2/patch_visitation.h>
#include <fboss/thrift_cow/visitors/DeltaVisitor.h>
#include <fboss/thrift_cow/visitors/TraverseHelper.h>

#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <functional>
#include <stdexcept>

namespace facebook::fboss::thrift_cow {

// Lower level construct to build out patch tree. If you just want to build a
// patch from two nodes, use PatchBuilder below
class PatchNodeBuilder {
 public:
  PatchNodeBuilder(ThriftTCType rootTC, bool incrementallyCompress);

  explicit PatchNodeBuilder(bool incrementallyCompress)
      : PatchNodeBuilder(ThriftTCType::STRUCTURE, incrementallyCompress) {}

  void onPathPush(const std::string& tok, ThriftTCType tc);

  void onPathPop(std::string&& tok, ThriftTCType /* tc */);

  template <typename Node>
  void setLeafPatch(Node& node) {
    if (node) {
      curPatch().set_val(node->encodeBuf(fsdb::OperProtocol::COMPACT));
    } else {
      curPatch().set_del();
    }
  }

  PatchNode moveRoot() {
    return std::move(root_);
  }

  PatchNode& curPatch() const {
    return curPath_.back();
  }

 private:
  void insertChild(PatchNode& node, const std::string& key, ThriftTCType tc);

  bool incrementallyCompress_;
  PatchNode root_;
  std::vector<std::reference_wrapper<PatchNode>> curPath_;
};

struct PatchBuilderTraverser : public TraverseHelper<PatchBuilderTraverser> {
  using Base = TraverseHelper<PatchBuilderTraverser>;

  explicit PatchBuilderTraverser(PatchNodeBuilder& nodeBuilder)
      : nodeBuilder_(nodeBuilder) {}

  bool shouldShortCircuitImpl(VisitorType visitorType) const {
    return false;
  }

  void onPushImpl(ThriftTCType tc) {
    const auto& newTok = path().back();
    nodeBuilder_.onPathPush(newTok, tc);
  }

  void onPopImpl(std::string&& popped, ThriftTCType tc) {
    nodeBuilder_.onPathPop(std::move(popped), tc);
  }

  PatchNodeBuilder& nodeBuilder() const {
    return nodeBuilder_;
  }

 private:
  PatchNodeBuilder& nodeBuilder_;
};

struct PatchBuilder {
  template <typename Node>
  static thrift_cow::Patch build(
      const std::shared_ptr<Node>& oldNode,
      const std::shared_ptr<Node>& newNode,
      const std::vector<std::string>& basePath,
      bool incrementallyCompress = false) {
    using TC = typename Node::TC;
    // TODO: validate type at path == Node
    thrift_cow::Patch patch;
    patch.basePath() = basePath;
    PatchNodeBuilder nodeBuilder(TCType<TC>, incrementallyCompress);

    auto processDelta = [&](const PatchBuilderTraverser& traverser,
                            auto /* oldNode */,
                            auto newNode,
                            thrift_cow::DeltaElemTag /* visitTag */) {
      traverser.nodeBuilder().setLeafPatch(newNode);
    };

    PatchBuilderTraverser traverser(nodeBuilder);
    thrift_cow::DeltaVisitor<TC>::visit(
        traverser,
        oldNode,
        newNode,
        thrift_cow::DeltaVisitOptions(
            thrift_cow::DeltaVisitMode::MINIMAL,
            DeltaVisitOrder::PARENTS_FIRST,
            true),
        std::move(processDelta));

    patch.patch() = nodeBuilder.moveRoot();
    return patch;
  }
};

} // namespace facebook::fboss::thrift_cow
