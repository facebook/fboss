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
    insertChild<SimpleTC>(curPatch, newTok);
  }

  template <ThriftSimpleTC SimpleTC>
  void onPopImpl(std::string&& /* popped */) {
    // TODO: prune empty paths on the way up
    curPath_.pop_back();
  }

  PatchNode& curPatch() const {
    return curPath_.back();
  }

 private:
  template <ThriftSimpleTC SimpleTC>
  void insertChild(PatchNode& node, const std::string& key) {
    apache::thrift::visit_union(
        node,
        [&](const apache::thrift::metadata::ThriftField& meta, auto& patch) {
          // TODO: construct child correctly
          PatchNode childPatch = constructEmptyPatch<SimpleTC>();
          auto insert = folly::overload(
              [](Empty&) -> PatchNode& {
                throw std::runtime_error("empty nodes cannot have children");
              },
              [](ByteBuffer&) -> PatchNode& {
                throw std::runtime_error("val nodes cannot have children");
              },
              [&](StructPatch& patch) -> PatchNode& {
                return patch.children()
                    ->try_emplace(
                        folly::to<apache::thrift::field_id_t>(key),
                        std::move(childPatch))
                    .first->second;
              },
              [&](ListPatch& patch) -> PatchNode& {
                return patch.children()
                    ->try_emplace(
                        folly::to<std::size_t>(key), std::move(childPatch))
                    .first->second;
              },
              [&](MapPatch& patch) -> PatchNode& {
                return patch.children()
                    ->try_emplace(key, std::move(childPatch))
                    .first->second;
              },
              [&](SetPatch& patch) -> PatchNode& {
                return patch.children()
                    ->try_emplace(key, std::move(childPatch))
                    .first->second;
              },
              [&](VariantPatch& patch) -> PatchNode& {
                patch.id() = folly::to<apache::thrift::field_id_t>(key);
                patch.child() = std::move(childPatch);
                return *patch.child();
              });
          curPath_.push_back(insert(patch));
        });
  }

  template <ThriftSimpleTC SimpleTC>
  PatchNode constructEmptyPatch() {
    PatchNode child;
    switch (SimpleTC) {
      case ThriftSimpleTC::PRIMITIVE:
        child.set_val();
        break;
      case ThriftSimpleTC::STRUCTURE:
        child.set_struct_node();
        break;
      case ThriftSimpleTC::VARIANT:
        child.set_variant_node();
        break;
      case ThriftSimpleTC::MAP:
        child.set_map_node();
        break;
      case ThriftSimpleTC::SET:
        child.set_set_node();
        break;
      case ThriftSimpleTC::LIST:
        child.set_list_node();
        break;
    }
    return child;
  }

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

    auto processDelta = [](const detail_pb::PatchBuilderTraverser& traverser,
                           auto oldNode,
                           auto newNode,
                           thrift_cow::DeltaElemTag visitTag) {
      if (newNode) {
        traverser.curPatch().set_val(
            newNode->encodeBuf(fsdb::OperProtocol::COMPACT));
      } else {
        traverser.curPatch().set_del();
      }
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
