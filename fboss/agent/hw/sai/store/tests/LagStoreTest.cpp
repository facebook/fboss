// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/LagApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

class LagStoreTest : public SaiStoreTest {
 public:
  void SetUp() override {
    SaiStoreTest::SetUp();
  }
  bool hasMember(
      facebook::fboss::LagSaiId lag,
      facebook::fboss::LagMemberSaiId member) {
    auto members = saiApiTable->lagApi().getAttribute(
        lag, facebook::fboss::SaiLagTraits::Attributes::PortList{});
    for (auto entry : members) {
      if (member == entry) {
        return true;
      }
    }
    return false;
  }

  facebook::fboss::LagSaiId createLag(std::string label, uint16_t vlan) {
    std::array<char, 32> labelValue{"lag0"};
    std::copy(label.begin(), label.end(), labelValue.data());
    facebook::fboss::SaiLagTraits::CreateAttributes c{labelValue, vlan};
    auto lagId =
        saiApiTable->lagApi().create<facebook::fboss::SaiLagTraits>(c, 0);

    std::array<char, 32> data{};
    std::copy(std::begin(label), std::end(label), std::begin(data));
    saiApiTable->lagApi().setAttribute(
        lagId, facebook::fboss::SaiLagTraits::Attributes::Label(data));
    return lagId;
  }

  facebook::fboss::LagMemberSaiId createLagMember(
      facebook::fboss::LagSaiId lagId,
      sai_object_id_t port) {
    return saiApiTable->lagApi().create<facebook::fboss::SaiLagMemberTraits>(
        {static_cast<sai_object_id_t>(lagId), port, true}, 0);
  }
};

TEST_F(LagStoreTest, setObject) {
  SaiStore s(0);
  s.reload();
  std::array<char, 32> labelValue{"lag0"};
  SaiLagTraits::Attributes::Label label{labelValue};
  auto lag = s.get<SaiLagTraits>().setObject(label, {labelValue, 1});
  std::vector<std::shared_ptr<SaiObject<SaiLagMemberTraits>>> members;
  for (auto i : {1, 2, 3, 4}) {
    SaiLagMemberTraits::AdapterHostKey adapterHostLey{lag->adapterKey(), i};
    SaiLagMemberTraits::CreateAttributes createAttrs{
        lag->adapterKey(),
        i,
        SaiLagMemberTraits::Attributes::EgressDisable{true}};
    members.push_back(
        s.get<SaiLagMemberTraits>().setObject(adapterHostLey, createAttrs));
  }
  for (auto member : members) {
    EXPECT_TRUE(hasMember(lag->adapterKey(), member->adapterKey()));
  }
}

TEST_F(LagStoreTest, updateObject) {
  SaiStore s(0);
  s.reload();
  std::array<char, 32> labelValue{"lag0"};
  SaiLagTraits::Attributes::Label label{labelValue};
  auto lag = s.get<SaiLagTraits>().setObject(label, {labelValue, 1});
  std::vector<std::shared_ptr<SaiObject<SaiLagMemberTraits>>> members;
  std::vector<LagMemberSaiId> memberIds;
  for (auto i : {1, 2, 3, 4}) {
    SaiLagMemberTraits::AdapterHostKey adapterHostLey{lag->adapterKey(), i};
    SaiLagMemberTraits::CreateAttributes createAttrs{
        lag->adapterKey(),
        i,
        SaiLagMemberTraits::Attributes::EgressDisable{true}};
    auto member =
        s.get<SaiLagMemberTraits>().setObject(adapterHostLey, createAttrs);
    memberIds.push_back(member->adapterKey());
    members.push_back(std::move(member));
  }
  for (auto memberId : memberIds) {
    EXPECT_TRUE(hasMember(lag->adapterKey(), memberId));
  }
  members.pop_back();
  for (int i = 0; i < memberIds.size(); i++) {
    if (i < 3) {
      EXPECT_TRUE(hasMember(lag->adapterKey(), memberIds[i]));
    } else {
      EXPECT_FALSE(hasMember(lag->adapterKey(), memberIds[i]));
    }
  }
}

TEST_F(LagStoreTest, loadLags) {
  auto id0 = createLag("a", 1000);
  auto id1 = createLag("b", 1001);

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiLagTraits>();

  auto got =
      store.get(SaiLagTraits::Attributes::Label{std::array<char, 32>{"a"}});
  EXPECT_EQ(got->adapterKey(), id0);
  EXPECT_EQ(GET_OPT_ATTR(Lag, PortVlanId, got->attributes()), 1000);
  got = store.get(SaiLagTraits::Attributes::Label{std::array<char, 32>{"b"}});
  EXPECT_EQ(got->adapterKey(), id1);
  EXPECT_EQ(GET_OPT_ATTR(Lag, PortVlanId, got->attributes()), 1001);
}

TEST_F(LagStoreTest, loadLagMembers) {
  auto id0 = createLag("c", 4001);
  auto member0 = createLagMember(id0, 1);
  auto member1 = createLagMember(id0, 2);
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiLagMemberTraits>();
  SaiLagMemberTraits::AdapterHostKey ahk0{
      static_cast<sai_object_id_t>(id0), static_cast<sai_object_id_t>(1)};
  auto got = store.get(ahk0);
  EXPECT_EQ(got->adapterKey(), member0);
  SaiLagMemberTraits::AdapterHostKey ahk1{
      static_cast<sai_object_id_t>(id0), static_cast<sai_object_id_t>(2)};
  got = store.get(ahk1);
  EXPECT_EQ(got->adapterKey(), member1);
}

TEST_F(LagStoreTest, toAndFromDynamic) {
  auto id0 = createLag("d", 4001);
  createLagMember(id0, 1);
  createLagMember(id0, 2);
  SaiStore s(0);
  s.reload();
  auto& lagStore = s.get<SaiLagTraits>();

  auto lagAdapterHostKey =
      SaiLagTraits::Attributes::Label{std::array<char, 32>{"d"}};
  auto got = lagStore.get(lagAdapterHostKey);

  auto k0 = got->adapterHostKeyToFollyDynamic();
  auto k1 = SaiObject<SaiLagTraits>::follyDynamicToAdapterHostKey(k0);
  EXPECT_EQ(k1, lagAdapterHostKey.value());

  auto ak2AhkJson = s.adapterKeys2AdapterHostKeysFollyDynamic();
  auto& lagAk2AhkJson = ak2AhkJson[saiObjectTypeToString(SAI_OBJECT_TYPE_LAG)];
  EXPECT_TRUE(lagAk2AhkJson.empty());
}
