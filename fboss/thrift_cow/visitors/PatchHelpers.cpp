// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/visitors/PatchHelpers.h"
#include "fboss/thrift_cow/gen-cpp2/patch_visitation.h"

#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace {
template <typename Patch>
void compressChildren(Patch& patch) {
  auto buf = apache::thrift::BinarySerializer::serialize<folly::IOBufQueue>(
      std::move(*patch.children()));
  patch.compressedChildren() = buf.moveAsValue();
  patch.children() = {};
}

template <typename Patch>
void decompressChildren(Patch& patch) {
  using ChildrenType = decltype(*patch.children());
  if (!patch.compressedChildren().has_value()) {
    return;
  }
  // All children should have been compressed
  DCHECK(patch.children()->size() == 0);
  auto buf = std::move(*patch.compressedChildren());
  apache::thrift::BinarySerializer::deserialize<ChildrenType>(
      &buf,
      *patch.children(),
      apache::thrift::ExternalBufferSharing::SHARE_EXTERNAL_BUFFER);
}
} // namespace

namespace facebook::fboss::thrift_cow {

void compressPatch(PatchNode& node) {
  apache::thrift::visit_union(
      node,
      [&](const apache::thrift::metadata::ThriftField& /* meta */,
          auto& patch) {
        auto compress = folly::overload(
            [](Empty& /* b */) {},
            [](ByteBuffer& /* b */) {},
            [&](StructPatch& patch) { compressChildren(patch); },
            [&](ListPatch& patch) { compressChildren(patch); },
            [&](MapPatch& patch) { compressChildren(patch); },
            [&](SetPatch& patch) { compressChildren(patch); },
            [&](VariantPatch& patch) {});
        compress(patch);
      });
}

void decompressPatch(PatchNode& node) {
  apache::thrift::visit_union(
      node,
      [&](const apache::thrift::metadata::ThriftField& /* meta */,
          auto& patch) {
        auto compress = folly::overload(
            [](Empty& /* b */) {},
            [](ByteBuffer& /* b */) {},
            [&](StructPatch& patch) { decompressChildren(patch); },
            [&](ListPatch& patch) { decompressChildren(patch); },
            [&](MapPatch& patch) { decompressChildren(patch); },
            [&](SetPatch& patch) { decompressChildren(patch); },
            [&](VariantPatch& patch) {});
        compress(patch);
      });
}

} // namespace facebook::fboss::thrift_cow
