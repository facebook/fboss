// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/nodes/Types.h"
#include "fboss/thrift_cow/nodes/tests/gen-cpp2/test_fatal_types.h"

#include "fboss/agent/state/DeltaFunctions.h"

#include <gtest/gtest.h>

namespace facebook::fboss::thrift_cow {

using k = test_tags::strings;

ADD_THRIFT_RESOLVER_MAPPING(TestStruct, DerivedTestStructNode);
class DerivedTestStructNode : public ThriftStructNode<TestStruct> {
 public:
  using Base = ThriftStructNode<TestStruct>;
  using Base::Base;

  int getInlineInt() const {
    return get<k::inlineInt>()->ref();
  }

  void setInlineInt(int i) {
    set<k::inlineInt>(i);
  }

 private:
  friend class CloneAllocator;
};

TEST(ThriftStructNodeResolverTests, DerivedTestStructNodeTest) {
  DerivedTestStructNode node;
  node.setInlineInt(123);
  EXPECT_EQ(node.getInlineInt(), 123);
}

ADD_THRIFT_RESOLVER_MAPPING(ParentTestStruct, DerivedParentTestNode);
class DerivedParentTestNode : public ThriftStructNode<ParentTestStruct> {
 public:
  using Base = ThriftStructNode<ParentTestStruct>;
  using Base::Base;

  std::shared_ptr<DerivedTestStructNode> getChild() const {
    return get<k::childStruct>();
  }

  void setChild(std::shared_ptr<DerivedTestStructNode> node) {
    ref<k::childStruct>() = node;
  }

 private:
  friend class CloneAllocator;
};

TEST(ThriftStructNodeResolverTests, DerivedParentTestStructNodeTest) {
  DerivedParentTestNode node;
  auto child = std::make_shared<DerivedTestStructNode>();
  child->setInlineInt(321);
  node.setChild(child);
  EXPECT_EQ(node.getChild()->getInlineInt(), 321);
}

template <typename Dervived, typename Resolver>
struct ThriftStructNodeMulti : public ThriftStructNode<TestStruct, Resolver> {
 public:
  using Base = ThriftStructNode<TestStruct, Resolver>;
  using Base::Base;

  int getInlineInt() const {
    return this->template get<k::inlineInt>()->ref();
  }

  void setInlineInt(int i) {
    this->template set<k::inlineInt>(i);
  }

 private:
  friend class CloneAllocator;
};

struct StructA;
struct StructB;
struct ThriftMapNodeA;
struct ThriftMapNodeB;

template <>
struct ResolveMemberType<StructA, k::mapA> : std::true_type {
  using type = ThriftMapNodeA;
};

template <>
struct ResolveMemberType<StructA, k::mapB> : std::true_type {
  using type = ThriftMapNodeB;
};

struct StructA : public ThriftStructNodeMulti<StructA, TypeIdentity<StructA>> {
  using Base = ThriftStructNodeMulti<StructA, TypeIdentity<StructA>>;
  using Base::Base;

 private:
  friend class CloneAllocator;
};

struct StructB : public ThriftStructNodeMulti<StructB, TypeIdentity<StructB>> {
  using Base = ThriftStructNodeMulti<StructB, TypeIdentity<StructB>>;
  using Base::Base;

 private:
  friend class CloneAllocator;
};

using String2StructTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::string,
    apache::thrift::type_class::structure>;
using String2TestStructType = std::map<std::string, TestStruct>;

template <typename...>
struct ConvertToStructATraits {
  using default_type = ThriftStructNode<TestStruct>;
  using struct_type = StructA;
  using type = std::shared_ptr<struct_type>;
  using isChild = std::true_type;
};

struct ThriftMapTraitsA {
  using TC = String2StructTypeClass;
  using Type = String2TestStructType;
  using KeyType = typename String2TestStructType::key_type;
  using KeyCompare = std::less<KeyType>;
  template <typename... T>
  using ConvertToNodeTraits = ConvertToStructATraits<T...>;
};

template <typename...>
struct ConvertToStructBTraits {
  using default_type = ThriftStructNode<TestStruct>;
  using struct_type = StructB;
  using type = std::shared_ptr<struct_type>;
  using isChild = std::true_type;
};

struct ThriftMapTraitsB {
  using TC = String2StructTypeClass;
  using Type = String2TestStructType;
  using KeyType = typename String2TestStructType::key_type;
  using KeyCompare = std::less<KeyType>;
  template <typename... T>
  using ConvertToNodeTraits = ConvertToStructBTraits<T...>;
};

template <typename Derived, typename MapTraits>
struct ThriftMapNodeMulti : ThriftMapNode<MapTraits, TypeIdentity<Derived>> {
  using Base = ThriftMapNode<MapTraits, TypeIdentity<Derived>>;
  using Base::Base;

 private:
  friend class CloneAllocator;
};

struct ThriftMapNodeA : ThriftMapNodeMulti<ThriftMapNodeA, ThriftMapTraitsA> {
  using Base = ThriftMapNodeMulti<ThriftMapNodeA, ThriftMapTraitsA>;
  using Base::Base;
  ThriftMapNodeA() {}

 private:
  friend class CloneAllocator;
};

struct ThriftMapNodeB : ThriftMapNodeMulti<ThriftMapNodeB, ThriftMapTraitsB> {
  using Base = ThriftMapNodeMulti<ThriftMapNodeB, ThriftMapTraitsB>;
  using Base::Base;

 private:
  friend class CloneAllocator;
};

TEST(ThriftStructNodeResolverTests, MultipleResolversSameStruct) {
  auto a = std::make_shared<StructA>();
  auto b = std::make_shared<StructB>();
  a->setInlineInt(100);
  b->setInlineInt(100);
  static_assert(!std::is_same_v<StructA, StructB>, "Same");
  EXPECT_EQ(a->toThrift(), b->toThrift());
  a->setInlineInt(200);
  EXPECT_NE(a->toThrift(), b->toThrift());
}

TEST(ThriftMapNodeResolverTests, MultipleResolversSameStruct) {
  auto mapA = std::make_shared<ThriftMapNodeA>();
  auto mapB = std::make_shared<ThriftMapNodeB>();

  auto t = mapA->toThrift();
  for (auto i = 1; i <= 5; i++) {
    mapA->insert(folly::to<std::string>(i), std::make_shared<StructA>());
    mapB->insert(folly::to<std::string>(i), std::make_shared<StructB>());
  }

  EXPECT_EQ(mapA->toThrift(), mapB->toThrift());
  mapA->insert(folly::to<std::string>(10), std::make_shared<StructA>());
  EXPECT_NE(mapA->toThrift(), mapB->toThrift());

  auto nodeA = mapA->at(std::string("1"));
  auto nodeB = mapB->at(std::string("1"));
  using TypeA = typename std::decay_t<decltype(nodeA)>::element_type;
  using TypeB = typename std::decay_t<decltype(nodeB)>::element_type;

  static_assert(!std::is_same_v<TypeA, TypeB>, "Same");
  static_assert(std::is_same_v<TypeA, StructA>, "Not Same");
  static_assert(std::is_same_v<TypeB, StructB>, "Not Same");

  auto mapA_0 = std::make_shared<ThriftMapNodeA>();

  for (auto i = 1; i <= 5; i++) {
    mapA_0->insert(folly::to<std::string>(i), std::make_shared<StructA>());
  }

  auto mapA_1 = mapA_0->clone();
  mapA_1->insert("6", std::make_shared<StructA>());

  auto delta = ThriftMapDelta<ThriftMapNodeA>(mapA_0.get(), mapA_1.get());

  auto addedFn = [](const auto& newNode) {
    using NodeType = std::decay_t<decltype(newNode)>;
    using StructType = typename NodeType::element_type;
    static_assert(std::is_same_v<StructType, StructA>, "Not Struct");
  };
  auto changedFun = [](const auto& /*oldNode*/, const auto& newNode) {
    using NodeType = std::decay_t<decltype(newNode)>;
    using StructType = typename NodeType::element_type;
    static_assert(std::is_same_v<StructType, StructA>, "Not Struct");
  };
  auto removedFun = [](const auto& oldNode) {
    using NodeType = std::decay_t<decltype(oldNode)>;
    using StructType = typename NodeType::element_type;
    static_assert(std::is_same_v<StructType, StructA>, "Not Struct");
  };

  facebook::fboss::DeltaFunctions::forEachAdded(delta, addedFn);
  facebook::fboss::DeltaFunctions::forEachRemoved(delta, removedFun);
  facebook::fboss::DeltaFunctions::forEachChanged(delta, changedFun);
}

TEST(ThriftMapNodeResolverTests, MultipleResolversInSameStruct) {
  TestStruct dataThrift{};
  dataThrift.mapA()->emplace("A0", TestStruct{});
  dataThrift.mapA()->emplace("A1", TestStruct{});

  dataThrift.mapB()->emplace("B0", TestStruct{});
  dataThrift.mapB()->emplace("B1", TestStruct{});

  ThriftStructNode<TestStruct> data0{dataThrift};
  auto mapA0 = data0.get<k::mapA>();
  auto mapB0 = data0.get<k::mapB>();
  static_assert(
      !std::is_same_v<
          std::decay_t<decltype(mapA0)::element_type>,
          ThriftMapNodeA>,
      "Unexpected");
  static_assert(
      !std::is_same_v<
          std::decay_t<decltype(mapB0)::element_type>,
          ThriftMapNodeB>,
      "Unexpected");

  StructA data1{dataThrift};
  auto mapA1 = data1.get<k::mapA>();
  auto mapB1 = data1.get<k::mapB>();

  static_assert(
      std::is_same_v<
          std::decay_t<decltype(mapA1)::element_type>,
          ThriftMapNodeA>,
      "Unexpected");
  static_assert(
      std::is_same_v<
          std::decay_t<decltype(mapB1)::element_type>,
          ThriftMapNodeB>,
      "Unexpected");
}
} // namespace facebook::fboss::thrift_cow
