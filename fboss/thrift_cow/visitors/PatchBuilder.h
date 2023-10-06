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

PatchNode constructEmptyPatch(ThriftTCType tc) {
  PatchNode child;
  switch (tc) {
    case ThriftTCType::PRIMITIVE:
      child.set_val();
      break;
    case ThriftTCType::STRUCTURE:
      child.set_struct_node();
      break;
    case ThriftTCType::VARIANT:
      child.set_variant_node();
      break;
    case ThriftTCType::MAP:
      child.set_map_node();
      break;
    case ThriftTCType::SET:
      child.set_set_node();
      break;
    case ThriftTCType::LIST:
      child.set_list_node();
      break;
  }
  return child;
}

struct PatchBuilderTraverser : public TraverseHelper<PatchBuilderTraverser> {
  using Base = TraverseHelper<PatchBuilderTraverser>;

  explicit PatchBuilderTraverser(PatchNode& rootPatch) {
    curPath_ = {rootPatch};
  }

  bool shouldShortCircuitImpl(VisitorType visitorType) const {
    return false;
  }

  void onPushImpl(ThriftTCType tc) {
    const auto& newTok = path().back();
    PatchNode& curPatch = curPath_.back();
    insertChild(curPatch, newTok, tc);
  }

  void onPopImpl(std::string&& popped, ThriftTCType /* tc */) {
    auto shouldPrune = true;
    apache::thrift::visit_union(
        curPath_.back().get(),
        [&](const apache::thrift::metadata::ThriftField& /* meta */,
            auto& patch) {
          auto checkChild = folly::overload(
              [](Empty& /* b */) -> bool { return false; },
              [](ByteBuffer& b) -> bool { return b.empty(); },
              [&](StructPatch& patch) -> bool {
                return patch.children()->size() == 0;
              },
              [&](ListPatch& patch) -> bool {
                return patch.children()->size() == 0;
              },
              [&](MapPatch& patch) -> bool {
                return patch.children()->size() == 0;
              },
              [&](SetPatch& patch) -> bool {
                return patch.children()->size() == 0;
              },
              [&](VariantPatch& patch) -> bool {
                return !patch.child().has_value();
              });
          shouldPrune = checkChild(patch);
        });
    curPath_.pop_back();
    if (shouldPrune) {
      apache::thrift::visit_union(
          curPath_.back().get(),
          [&](const apache::thrift::metadata::ThriftField& /* meta */,
              auto& patch) {
            auto removeChild = folly::overload(
                [](ByteBuffer&) -> bool {
                  throw std::runtime_error(
                      "val nodes should never be a parent");
                },
                [](Empty&) -> bool {
                  throw std::runtime_error(
                      "del nodes should never be a parent");
                },
                [&](StructPatch& patch) {
                  patch.children()->erase(
                      folly::to<apache::thrift::field_id_t>(std::move(popped)));
                },
                [&](ListPatch& patch) {
                  patch.children()->erase(
                      folly::to<std::size_t>(std::move(popped)));
                },
                [&](MapPatch& patch) {
                  patch.children()->erase(std::move(popped));
                },
                [&](SetPatch& patch) {
                  patch.children()->erase(std::move(popped));
                },
                [&](VariantPatch& patch) {
                  patch.id() = 0;
                  patch.child().reset();
                });
            removeChild(patch);
          });
    }
  }

  PatchNode& curPatch() const {
    return curPath_.back();
  }

 private:
  void insertChild(PatchNode& node, const std::string& key, ThriftTCType tc) {
    apache::thrift::visit_union(
        node,
        [&](const apache::thrift::metadata::ThriftField& /* meta */,
            auto& patch) {
          PatchNode childPatch = constructEmptyPatch(tc);
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

  std::vector<std::reference_wrapper<PatchNode>> curPath_;
};

} // namespace detail_pb

struct PatchBuilder {
  template <typename Node>
  static thrift_cow::Patch build(
      const std::shared_ptr<Node>& oldNode,
      const std::shared_ptr<Node>& newNode,
      const std::vector<std::string>& basePath) {
    using TC = typename Node::TC;
    // TODO: validate type at path == Node
    thrift_cow::Patch patch;
    patch.basePath() = basePath;
    patch.patch() = detail_pb::constructEmptyPatch(TCType<TC>);

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

    detail_pb::PatchBuilderTraverser traverser(*patch.patch());
    thrift_cow::DeltaVisitor<TC>::visit(
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
