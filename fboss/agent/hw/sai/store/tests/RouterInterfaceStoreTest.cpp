/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/RouterInterfaceApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/LoggingUtil.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

class RouterInterfaceStoreTest : public SaiStoreTest {
 public:
  RouterInterfaceSaiId createRouterInterface(
      sai_object_id_t vlanId,
      const folly::MacAddress& mac,
      sai_uint32_t mtu) {
    return saiApiTable->routerInterfaceApi().create<SaiRouterInterfaceTraits>(
        {0, SAI_ROUTER_INTERFACE_TYPE_VLAN, vlanId, mac, mtu}, 0);
  }
};

TEST_F(RouterInterfaceStoreTest, loadRouterInterfaces) {
  auto srcMac1 = folly::MacAddress{"41:41:41:41:41:41"};
  auto srcMac2 = folly::MacAddress{"42:42:42:42:42:42"};
  auto routerInterfaceSaiId1 = createRouterInterface(41, srcMac1, 1514);
  auto routerInterfaceSaiId2 = createRouterInterface(42, srcMac2, 9000);

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiRouterInterfaceTraits>();

  SaiRouterInterfaceTraits::AdapterHostKey k1{0, 41};
  SaiRouterInterfaceTraits::AdapterHostKey k2{0, 42};
  auto got = store.get(k1);
  EXPECT_EQ(got->adapterKey(), routerInterfaceSaiId1);
  got = store.get(k2);
  EXPECT_EQ(got->adapterKey(), routerInterfaceSaiId2);
}

TEST_F(RouterInterfaceStoreTest, routerInterfaceLoadCtor) {
  folly::MacAddress srcMac{"41:41:41:41:41:41"};
  auto routerInterfaceSaiId = createRouterInterface(41, srcMac, 9000);

  SaiObject<SaiRouterInterfaceTraits> obj(routerInterfaceSaiId);
  EXPECT_EQ(obj.adapterKey(), routerInterfaceSaiId);
  EXPECT_EQ(GET_ATTR(RouterInterface, VlanId, obj.attributes()), 41);
  EXPECT_EQ(GET_OPT_ATTR(RouterInterface, SrcMac, obj.attributes()), srcMac);
  EXPECT_EQ(GET_OPT_ATTR(RouterInterface, Mtu, obj.attributes()), 9000);
}

TEST_F(RouterInterfaceStoreTest, routerInterfaceCreateCtor) {
  folly::MacAddress srcMac{"41:41:41:41:41:41"};
  SaiRouterInterfaceTraits::AdapterHostKey k{0, 41};
  SaiRouterInterfaceTraits::CreateAttributes c{
      0, SAI_ROUTER_INTERFACE_TYPE_VLAN, 41, srcMac, 9000};

  SaiObject<SaiRouterInterfaceTraits> obj(k, c, 0);
  EXPECT_EQ(GET_ATTR(RouterInterface, VlanId, obj.attributes()), 41);
  EXPECT_EQ(GET_OPT_ATTR(RouterInterface, SrcMac, obj.attributes()), srcMac);
  EXPECT_EQ(GET_OPT_ATTR(RouterInterface, Mtu, obj.attributes()), 9000);
}

TEST_F(RouterInterfaceStoreTest, routerInterfaceSetSrcMac) {
  folly::MacAddress srcMac{"41:41:41:41:41:41"};
  SaiRouterInterfaceTraits::AdapterHostKey k{0, 41};
  SaiRouterInterfaceTraits::CreateAttributes c{
      0, SAI_ROUTER_INTERFACE_TYPE_VLAN, 41, srcMac, 9000};

  SaiObject<SaiRouterInterfaceTraits> obj(k, c, 0);
  EXPECT_EQ(GET_ATTR(RouterInterface, VlanId, obj.attributes()), 41);
  EXPECT_EQ(GET_OPT_ATTR(RouterInterface, SrcMac, obj.attributes()), srcMac);
  EXPECT_EQ(GET_OPT_ATTR(RouterInterface, Mtu, obj.attributes()), 9000);

  folly::MacAddress srcMac2{"42:42:42:42:42:42"};
  SaiRouterInterfaceTraits::CreateAttributes newAttrs{
      0, SAI_ROUTER_INTERFACE_TYPE_VLAN, 41, srcMac2, 1514};
  obj.setAttributes(newAttrs);
  EXPECT_EQ(GET_ATTR(RouterInterface, VlanId, obj.attributes()), 41);
  EXPECT_EQ(GET_OPT_ATTR(RouterInterface, SrcMac, obj.attributes()), srcMac2);
  EXPECT_EQ(GET_OPT_ATTR(RouterInterface, Mtu, obj.attributes()), 1514);
}

TEST_F(RouterInterfaceStoreTest, serDeserTest) {
  auto routerInterfaceSaiId =
      createRouterInterface(41, folly::MacAddress{"41:41:41:41:41:41"}, 1514);

  verifyAdapterKeySerDeser<SaiRouterInterfaceTraits>({routerInterfaceSaiId});
}

TEST_F(RouterInterfaceStoreTest, routerFormatTest) {
  folly::MacAddress srcMac{"41:41:41:41:41:41"};
  SaiRouterInterfaceTraits::AdapterHostKey k{0, 41};
  SaiRouterInterfaceTraits::CreateAttributes c{
      0, SAI_ROUTER_INTERFACE_TYPE_VLAN, 41, srcMac, 9000};
  SaiObject<SaiRouterInterfaceTraits> obj(k, c, 0);
  auto expected =
      "RouterInterfaceSaiId(0): "
      "(VirtualRouterId: 0, Type: 1, VlanId: 41, "
      "SrcMac: 41:41:41:41:41:41, Mtu: 9000)";
  EXPECT_EQ(expected, fmt::format("{}", obj));
}

TEST_F(RouterInterfaceStoreTest, toStrTest) {
  std::ignore =
      createRouterInterface(41, folly::MacAddress{"41:41:41:41:41:41"}, 1514);
  verifyToStr<SaiRouterInterfaceTraits>();
}
