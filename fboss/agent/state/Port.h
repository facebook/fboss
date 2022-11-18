/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/PortPgConfig.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/mka_service/if/gen-cpp2/mka_structs_types.h"

#include <boost/container/flat_map.hpp>
#include <map>
#include <string>
#include <vector>

namespace facebook::fboss {

class SwitchState;

struct PortFields : public ThriftyFields<PortFields, state::PortFields> {
  struct VlanInfo {
    explicit VlanInfo(bool emitTags) : tagged(emitTags) {}
    bool operator==(const VlanInfo& other) const {
      return tagged == other.tagged;
    }
    bool operator!=(const VlanInfo& other) const {
      return !(*this == other);
    }
    state::VlanInfo toThrift() const;
    static VlanInfo fromThrift(state::VlanInfo const&);
    bool tagged;
  };

  using VlanMembership = boost::container::flat_map<VlanID, VlanInfo>;
  using LLDPValidations = std::map<cfg::LLDPTag, std::string>;

  enum class OperState {
    DOWN = 0,
    UP = 1,
  };

  PortFields(PortID id, std::string name) : id(id), name(name) {}

  bool operator==(const PortFields& other) const;

  template <typename Fn>
  void forEachChild(Fn /*fn*/) {}

  static PortFields fromThrift(state::PortFields const& pf);
  state::PortFields toThrift() const override;

  // Port migration is complete
  static PortFields fromFollyDynamicLegacy(folly::dynamic const& dyn) {
    return fromFollyDynamic(dyn);
  }
  folly::dynamic toFollyDynamicLegacy() const {
    return toFollyDynamic();
  }

  struct MKASakKey {
    bool operator<(const MKASakKey& r) const {
      return std::tie(*sci.macAddress(), *sci.port(), associationNum) <
          std::tie(*r.sci.macAddress(), *r.sci.port(), r.associationNum);
    }
    bool operator==(const MKASakKey& r) const {
      return std::tie(*sci.macAddress(), *sci.port(), associationNum) ==
          std::tie(*r.sci.macAddress(), *r.sci.port(), r.associationNum);
    }
    mka::MKASci sci;
    int associationNum;
  };
  using RxSaks = std::map<MKASakKey, mka::MKASak>;

  state::RxSak rxSakToThrift(const MKASakKey& sakKey, const mka::MKASak& sak)
      const;
  static std::pair<MKASakKey, mka::MKASak> rxSakFromThrift(state::RxSak rxSak);

  const PortID id{0};
  std::string name;
  std::string description;
  cfg::PortState adminState{cfg::PortState::DISABLED}; // is the port enabled
  OperState operState{OperState::DOWN}; // is the port actually up
  std::optional<phy::LinkFaultStatus> iPhyLinkFaultStatus;
  phy::PortPrbsState asicPrbs = phy::PortPrbsState();
  phy::PortPrbsState gbSystemPrbs = phy::PortPrbsState();
  phy::PortPrbsState gbLinePrbs = phy::PortPrbsState();
  VlanID ingressVlan{0};
  cfg::PortSpeed speed{cfg::PortSpeed::DEFAULT};
  cfg::PortPause pause;
  std::optional<cfg::PortPfc> pfc;
  std::optional<std::vector<PfcPriority>> pfcPriorities;
  VlanMembership vlans;
  // settings for ingress/egress sFlow sampling rate; we sample every 1:N'th
  // packets randomly based on those settings. Zero means no sampling.
  int64_t sFlowIngressRate{0};
  int64_t sFlowEgressRate{0};
  // specifies whether sFlow sampled packets should be forwarded to the CPU or
  // to remote Mirror destinations
  std::optional<cfg::SampleDestination> sampleDest;
  QueueConfig queues;
  std::optional<PortPgConfigs> pgConfigs;
  cfg::PortLoopbackMode loopbackMode{cfg::PortLoopbackMode::NONE};
  std::optional<std::string> ingressMirror;
  std::optional<std::string> egressMirror;
  std::optional<std::string> qosPolicy;
  LLDPValidations expectedLLDPValues;
  std::vector<cfg::AclLookupClass> lookupClassesToDistrubuteTrafficOn;
  cfg::PortProfileID profileID{cfg::PortProfileID::PROFILE_DEFAULT};
  // Default value from switch_config.thrift
  int32_t maxFrameSize{cfg::switch_config_constants::DEFAULT_PORT_MTU()};
  // Currently we use PlatformPort to fetch such config when we are trying to
  // program Hardware. This config is from PlatformMapping. Since we need to
  // program all this config into Hardware, it's a good practice to use a
  // switch state to drive HwSwitch programming.
  phy::ProfileSideConfig profileConfig;
  std::vector<phy::PinConfig> pinConfigs;
  // Due to we also use SW Port to program phy ports, which have system and line
  // profileConfig and pinConfigs, using these two new fields to represent the
  // configs of line side if needed
  std::optional<phy::ProfileSideConfig> lineProfileConfig;
  std::optional<std::vector<phy::PinConfig>> linePinConfigs;
  // Macsec configs
  RxSaks rxSecureAssociationKeys;
  std::optional<mka::MKASak> txSecureAssociationKey;
  bool macsecDesired{false};
  bool dropUnencrypted{false};
  cfg::PortType portType = cfg::PortType::INTERFACE_PORT;
};

/*
 * Port stores state about one of the physical ports on the switch.
 */
class Port : public ThriftyBaseT<state::PortFields, Port, PortFields> {
 public:
  using VlanInfo = PortFields::VlanInfo;
  using VlanMembership = PortFields::VlanMembership;
  using OperState = PortFields::OperState;
  using LLDPValidations = PortFields::LLDPValidations;
  using MKASakKey = PortFields::MKASakKey;
  using RxSaks = PortFields::RxSaks;

  Port(PortID id, const std::string& name);

  PortID getID() const {
    return getFields()->id;
  }

  const std::string& getName() const {
    return getFields()->name;
  }

  void setName(const std::string& name) {
    writableFields()->name = name;
  }

  const std::string& getDescription() const {
    return getFields()->description;
  }

  void setDescription(const std::string& description) {
    writableFields()->description = description;
  }

  phy::PortPrbsState getAsicPrbs() const {
    return getFields()->asicPrbs;
  }

  void setAsicPrbs(phy::PortPrbsState prbsState) {
    writableFields()->asicPrbs = prbsState;
  }

  phy::PortPrbsState getGbSystemPrbs() const {
    return getFields()->gbSystemPrbs;
  }

  void setGbSystemPrbs(phy::PortPrbsState prbsState) {
    writableFields()->gbSystemPrbs = prbsState;
  }

  phy::PortPrbsState getGbLinePrbs() const {
    return getFields()->gbLinePrbs;
  }

  void setGbLinePrbs(phy::PortPrbsState prbsState) {
    writableFields()->gbLinePrbs = prbsState;
  }

  cfg::PortState getAdminState() const {
    return getFields()->adminState;
  }

  cfg::PortType getPortType() const {
    return getFields()->portType;
  }
  void setPortType(cfg::PortType portType) {
    writableFields()->portType = portType;
  }

  void setAdminState(cfg::PortState adminState) {
    writableFields()->adminState = adminState;
  }

  std::optional<phy::LinkFaultStatus> getIPhyLinkFaultStatus() const {
    return getFields()->iPhyLinkFaultStatus;
  }

  void setIPhyLinkFaultStatus(std::optional<phy::LinkFaultStatus> faultStatus) {
    writableFields()->iPhyLinkFaultStatus = faultStatus;
  }

  OperState getOperState() const {
    return getFields()->operState;
  }

  void setOperState(bool isUp) {
    writableFields()->operState = isUp ? OperState::UP : OperState::DOWN;
  }

  bool isEnabled() const {
    return getFields()->adminState == cfg::PortState::ENABLED;
  }

  bool isUp() const {
    return getFields()->operState == OperState::UP;
  }

  std::optional<mka::MKASak> getTxSak() const {
    return getFields()->txSecureAssociationKey;
  }
  void setTxSak(std::optional<mka::MKASak> txSak) {
    writableFields()->txSecureAssociationKey = txSak;
  }
  const RxSaks& getRxSaks() const {
    return getFields()->rxSecureAssociationKeys;
  }
  void setRxSaks(const RxSaks& rxSaks) {
    writableFields()->rxSecureAssociationKeys = rxSaks;
  }

  bool getMacsecDesired() const {
    return getFields()->macsecDesired;
  }
  void setMacsecDesired(bool macsecDesired) {
    writableFields()->macsecDesired = macsecDesired;
  }

  bool getDropUnencrypted() const {
    return getFields()->dropUnencrypted;
  }
  void setDropUnencrypted(bool dropUnencrypted) {
    writableFields()->dropUnencrypted = dropUnencrypted;
  }

  /**
   * Tells you Oper+Admin state of port. Will be UP only if both admin and
   * oper state is UP.
   */
  bool isPortUp() const {
    return isEnabled() && isUp();
  }

  const VlanMembership& getVlans() const {
    return getFields()->vlans;
  }
  void setVlans(VlanMembership vlans) {
    writableFields()->vlans.swap(vlans);
  }

  void addVlan(VlanID id, bool tagged) {
    writableFields()->vlans.emplace(std::make_pair(id, VlanInfo(tagged)));
  }

  const QueueConfig& getPortQueues() const {
    return getFields()->queues;
  }

  bool hasValidPortQueues() const {
    constexpr auto kDefaultProbability = 100;
    for (const auto& portQueue : getFields()->queues) {
      const auto& aqms = portQueue->get<switch_state_tags::aqms>();
      if (!aqms) {
        continue;
      }
      for (const auto& entry : std::as_const(*aqms)) {
        // THRIFT_COPY
        auto behavior = entry->cref<switch_config_tags::behavior>()->toThrift();
        auto detection =
            entry->cref<switch_config_tags::detection>()->toThrift();
        if (behavior == facebook::fboss::cfg::QueueCongestionBehavior::ECN &&
            detection.linear_ref()->probability() != kDefaultProbability) {
          return false;
        }
      }
    }
    return true;
  }

  void resetPortQueues(QueueConfig queues) {
    writableFields()->queues.swap(queues);
  }

  std::optional<const PortPgConfigs> getPortPgConfigs() {
    return getFields()->pgConfigs;
  }

  void resetPgConfigs(std::optional<PortPgConfigs>& pgConfigs) {
    writableFields()->pgConfigs.swap(pgConfigs);
  }

  VlanID getIngressVlan() const {
    return getFields()->ingressVlan;
  }
  void setIngressVlan(VlanID id) {
    writableFields()->ingressVlan = id;
  }

  cfg::PortSpeed getSpeed() const {
    return getFields()->speed;
  }
  void setSpeed(cfg::PortSpeed speed) {
    writableFields()->speed = speed;
  }

  cfg::PortProfileID getProfileID() const {
    return getFields()->profileID;
  }
  void setProfileId(cfg::PortProfileID profileID) {
    writableFields()->profileID = profileID;
  }

  cfg::PortPause getPause() const {
    return getFields()->pause;
  }
  void setPause(cfg::PortPause pause) {
    writableFields()->pause = pause;
  }
  std::optional<cfg::PortPfc> getPfc() const {
    return getFields()->pfc;
  }
  void setPfc(std::optional<cfg::PortPfc>& pfc) {
    writableFields()->pfc = pfc;
  }

  std::optional<std::vector<PfcPriority>> getPfcPriorities() const {
    return getFields()->pfcPriorities;
  }
  void setPfcPriorities(std::optional<std::vector<PfcPriority>>& pri) {
    writableFields()->pfcPriorities = pri;
  }

  int32_t getMaxFrameSize() const {
    return getFields()->maxFrameSize;
  }
  void setMaxFrameSize(int32_t maxFrameSize) {
    writableFields()->maxFrameSize = maxFrameSize;
  }

  cfg::PortLoopbackMode getLoopbackMode() const {
    return getFields()->loopbackMode;
  }
  void setLoopbackMode(cfg::PortLoopbackMode loopbackMode) {
    writableFields()->loopbackMode = loopbackMode;
  }

  int64_t getSflowIngressRate() const {
    return getFields()->sFlowIngressRate;
  }
  void setSflowIngressRate(int64_t ingressRate) {
    writableFields()->sFlowIngressRate = ingressRate;
  }

  int64_t getSflowEgressRate() const {
    return getFields()->sFlowEgressRate;
  }
  void setSflowEgressRate(int64_t egressRate) {
    writableFields()->sFlowEgressRate = egressRate;
  }

  std::optional<cfg::SampleDestination> getSampleDestination() const {
    return getFields()->sampleDest;
  }

  void setSampleDestination(
      const std::optional<cfg::SampleDestination>& sampleDest) {
    writableFields()->sampleDest = sampleDest;
  }

  std::optional<std::string> getIngressMirror() const {
    return getFields()->ingressMirror;
  }

  void setIngressMirror(const std::optional<std::string>& mirror) {
    writableFields()->ingressMirror = mirror;
  }

  std::optional<std::string> getEgressMirror() const {
    return getFields()->egressMirror;
  }

  void setEgressMirror(const std::optional<std::string>& mirror) {
    writableFields()->egressMirror = mirror;
  }

  std::optional<std::string> getQosPolicy() const {
    return getFields()->qosPolicy;
  }

  void setQosPolicy(const std::optional<std::string>& qosPolicy) {
    writableFields()->qosPolicy = qosPolicy;
  }

  const LLDPValidations& getLLDPValidations() const {
    return getFields()->expectedLLDPValues;
  }

  void setExpectedLLDPValues(LLDPValidations vals) {
    writableFields()->expectedLLDPValues.swap(vals);
  }

  const std::vector<cfg::AclLookupClass>&
  getLookupClassesToDistributeTrafficOn() const {
    return getFields()->lookupClassesToDistrubuteTrafficOn;
  }

  void setLookupClassesToDistributeTrafficOn(
      const std::vector<cfg::AclLookupClass>&
          lookupClassesToDistrubuteTrafficOn) {
    writableFields()->lookupClassesToDistrubuteTrafficOn =
        lookupClassesToDistrubuteTrafficOn;
  }

  const phy::ProfileSideConfig& getProfileConfig() const {
    return getFields()->profileConfig;
  }
  void setProfileConfig(const phy::ProfileSideConfig& profileCfg) {
    writableFields()->profileConfig = profileCfg;
  }

  const std::vector<phy::PinConfig>& getPinConfigs() const {
    return getFields()->pinConfigs;
  }
  void resetPinConfigs(std::vector<phy::PinConfig> pinCfgs) {
    writableFields()->pinConfigs.swap(pinCfgs);
  }

  std::optional<phy::ProfileSideConfig> getLineProfileConfig() const {
    return getFields()->lineProfileConfig;
  }
  void setLineProfileConfig(const phy::ProfileSideConfig& profileCfg) {
    writableFields()->lineProfileConfig = profileCfg;
  }

  std::optional<std::vector<phy::PinConfig>> getLinePinConfigs() const {
    return getFields()->linePinConfigs;
  }
  void resetLinePinConfigs(std::vector<phy::PinConfig> pinCfgs) {
    writableFields()->linePinConfigs = pinCfgs;
  }

  Port* modify(std::shared_ptr<SwitchState>* state);

  void fillPhyInfo(phy::PhyInfo* phyInfo);

 private:
  // Inherit the constructors required for clone()
  using ThriftyBaseT<state::PortFields, Port, PortFields>::ThriftyBaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
