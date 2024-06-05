// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/visitors/PatchBuilder.h"
#include "fboss/thrift_cow/visitors/PatchHelpers.h"

#include <thrift/lib/cpp2/protocol/Serializer.h>

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

} // namespace detail_pb

PatchNodeBuilder::PatchNodeBuilder(
    ThriftTCType rootTC,
    fsdb::OperProtocol protocol,
    bool incrementallyCompress)
    : protocol_(protocol), incrementallyCompress_(incrementallyCompress) {
  root_ = detail_pb::constructEmptyPatch(rootTC);
  curPath_ = {root_};
}

void PatchNodeBuilder::onPathPush(const std::string& tok, ThriftTCType tc) {
  PatchNode& curPatch = curPath_.back();
  insertChild(curPatch, tok, tc);
}

void PatchNodeBuilder::onPathPop(std::string&& tok, ThriftTCType /* tc */) {
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
  if (!shouldPrune && incrementallyCompress_) {
    compressPatch(curPath_.back().get());
  }
  XLOG_IF(DBG5, !shouldPrune) << "Keeping patch at tok " << tok;
  curPath_.pop_back();
  if (shouldPrune) {
    apache::thrift::visit_union(
        curPath_.back().get(),
        [&](const apache::thrift::metadata::ThriftField& /* meta */,
            auto& patch) {
          auto removeChild = folly::overload(
              [](ByteBuffer&) -> bool {
                throw std::runtime_error("val nodes should never be a parent");
              },
              [](Empty&) -> bool {
                throw std::runtime_error("del nodes should never be a parent");
              },
              [&](StructPatch& patch) {
                patch.children()->erase(
                    folly::to<apache::thrift::field_id_t>(std::move(tok)));
              },
              [&](ListPatch& patch) {
                patch.children()->erase(folly::to<std::size_t>(std::move(tok)));
              },
              [&](MapPatch& patch) { patch.children()->erase(std::move(tok)); },
              [&](SetPatch& patch) { patch.children()->erase(std::move(tok)); },
              [&](VariantPatch& patch) {
                patch.id() = 0;
                patch.child().reset();
              });
          removeChild(patch);
        });
  }
}

void PatchNodeBuilder::insertChild(
    PatchNode& node,
    const std::string& key,
    ThriftTCType tc) {
  apache::thrift::visit_union(
      node,
      [&](const apache::thrift::metadata::ThriftField& /* meta */,
          auto& patch) {
        PatchNode childPatch = detail_pb::constructEmptyPatch(tc);
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

void PatchBuilderTraverser::onPushImpl(ThriftTCType tc) {
  const auto& newTok = path().back();
  nodeBuilder_.onPathPush(newTok, tc);
}

void PatchBuilderTraverser::onPopImpl(std::string&& popped, ThriftTCType tc) {
  nodeBuilder_.onPathPop(std::move(popped), tc);
}

} // namespace facebook::fboss::thrift_cow
