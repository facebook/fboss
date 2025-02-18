#pragma once

#include <unordered_map>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "fboss/qsfp_service/module/cmis/CmisModule.h"

namespace facebook {
namespace fboss {
using SmfMediaInterfaceMap =
    std::unordered_map<SMFMediaInterfaceCode, MediaInterfaceCode>;

template <typename InterfaceCode>
using SpeedApplicationMap = std::
    unordered_map<facebook::fboss::cfg::PortSpeed, std::vector<InterfaceCode>>;
using SmfSpeedApplicationMap = SpeedApplicationMap<SMFMediaInterfaceCode>;

using SmfValidSpeedCombinations = std::vector<
    std::array<SMFMediaInterfaceCode, CmisModule::kMaxOsfpNumLanes>>;

class CmisHelper final {
 private:
  // Disable Construction of this class.
  CmisHelper() = delete;
  ~CmisHelper() = delete;
  CmisHelper(CmisHelper const&) = delete;
  CmisHelper& operator=(CmisHelper const&) = delete;

 public:
  static const SmfSpeedApplicationMap& getSmfSpeedApplicationMapping() {
    // Store the mapping between port speed and SMF MediaApplicationCode.
    // We use the module Media Interface ID, which is located at the second byte
    // of the APPLICATION_ADVERTISING field, as Application ID here, which is
    // the code from Table4-7 SM media interface codes in Sff-8024.
    static const SmfSpeedApplicationMap smfSpeedApplicationMapping_ = {
        {facebook::fboss::cfg::PortSpeed::FIFTYTHREEPOINTONETWOFIVEG,
         {SMFMediaInterfaceCode::FR4_200G}},
        {facebook::fboss::cfg::PortSpeed::HUNDREDG,
         {SMFMediaInterfaceCode::CWDM4_100G, SMFMediaInterfaceCode::FR1_100G}},
        {facebook::fboss::cfg::PortSpeed::HUNDREDANDSIXPOINTTWOFIVEG,
         {SMFMediaInterfaceCode::FR1_100G}},
        {facebook::fboss::cfg::PortSpeed::TWOHUNDREDG,
         {SMFMediaInterfaceCode::FR4_200G}},
        {facebook::fboss::cfg::PortSpeed::FOURHUNDREDG,
         {SMFMediaInterfaceCode::FR4_400G,
          SMFMediaInterfaceCode::LR4_10_400G,
          SMFMediaInterfaceCode::DR4_400G}},
        {
            facebook::fboss::cfg::PortSpeed::EIGHTHUNDREDG,
            {SMFMediaInterfaceCode::FR8_800G},
        }};
    return smfSpeedApplicationMapping_;
  }

  /*
   * getInterfaceCode
   *
   * a Function used to get the Supported interface codes supported
   * for each speed. One of the values in the returned vector will be
   * programmed to the hardware, so returning a uint8_t
   */
  static std::vector<uint8_t> getInterfaceCode(
      facebook::fboss::cfg::PortSpeed speed) {
    std::vector<uint8_t> interfaceCodes;
    const auto& smfSpeedMapping = getSmfSpeedApplicationMapping();
    const auto itr = smfSpeedMapping.find(speed);
    if (itr != smfSpeedMapping.end()) {
      for (auto& val : itr->second) {
        interfaceCodes.push_back(static_cast<uint8_t>(val));
      }
      return interfaceCodes;
    }
    return {};
  }

  static MediaInterfaceCode getMediaInterfaceCode(
      SMFMediaInterfaceCode mediaInterfaceCode) {
    static const SmfMediaInterfaceMap smfMediaInterfaceMapping_ = {
        {SMFMediaInterfaceCode::CWDM4_100G, MediaInterfaceCode::CWDM4_100G},
        {SMFMediaInterfaceCode::FR1_100G, MediaInterfaceCode::FR1_100G},
        {SMFMediaInterfaceCode::FR4_200G, MediaInterfaceCode::FR4_200G},
        {SMFMediaInterfaceCode::FR4_400G, MediaInterfaceCode::FR4_400G},
        {SMFMediaInterfaceCode::LR4_10_400G, MediaInterfaceCode::LR4_400G_10KM},
        {SMFMediaInterfaceCode::DR4_400G, MediaInterfaceCode::DR4_400G},
        {SMFMediaInterfaceCode::FR8_800G, MediaInterfaceCode::FR8_800G},
    };
    const auto itr = smfMediaInterfaceMapping_.find(mediaInterfaceCode);
    if (itr != smfMediaInterfaceMapping_.end()) {
      return itr->second;
    }
    return MediaInterfaceCode::UNKNOWN;
  }

  static const SmfValidSpeedCombinations& getSmfValidSpeedCombinations() {
    static const SmfValidSpeedCombinations smfOsfpValidSpeedCombinations_ = {
        {
            // 2x400G-FR4
            SMFMediaInterfaceCode::FR4_400G,
            SMFMediaInterfaceCode::FR4_400G,
            SMFMediaInterfaceCode::FR4_400G,
            SMFMediaInterfaceCode::FR4_400G,
            SMFMediaInterfaceCode::FR4_400G,
            SMFMediaInterfaceCode::FR4_400G,
            SMFMediaInterfaceCode::FR4_400G,
            SMFMediaInterfaceCode::FR4_400G,
        },
        {
            // 2x200G-FR4
            SMFMediaInterfaceCode::FR4_200G,
            SMFMediaInterfaceCode::FR4_200G,
            SMFMediaInterfaceCode::FR4_200G,
            SMFMediaInterfaceCode::FR4_200G,
            SMFMediaInterfaceCode::FR4_200G,
            SMFMediaInterfaceCode::FR4_200G,
            SMFMediaInterfaceCode::FR4_200G,
            SMFMediaInterfaceCode::FR4_200G,
        },
        {
            // 400G-FR4 + 200G-FR4
            SMFMediaInterfaceCode::FR4_400G,
            SMFMediaInterfaceCode::FR4_400G,
            SMFMediaInterfaceCode::FR4_400G,
            SMFMediaInterfaceCode::FR4_400G,
            SMFMediaInterfaceCode::FR4_200G,
            SMFMediaInterfaceCode::FR4_200G,
            SMFMediaInterfaceCode::FR4_200G,
            SMFMediaInterfaceCode::FR4_200G,
        },
        {
            // 200G-FR4 + 400G-FR4
            SMFMediaInterfaceCode::FR4_200G,
            SMFMediaInterfaceCode::FR4_200G,
            SMFMediaInterfaceCode::FR4_200G,
            SMFMediaInterfaceCode::FR4_200G,
            SMFMediaInterfaceCode::FR4_400G,
            SMFMediaInterfaceCode::FR4_400G,
            SMFMediaInterfaceCode::FR4_400G,
            SMFMediaInterfaceCode::FR4_400G,
        },
        {
            // 8x100G-FR1
            SMFMediaInterfaceCode::FR1_100G,
            SMFMediaInterfaceCode::FR1_100G,
            SMFMediaInterfaceCode::FR1_100G,
            SMFMediaInterfaceCode::FR1_100G,
            SMFMediaInterfaceCode::FR1_100G,
            SMFMediaInterfaceCode::FR1_100G,
            SMFMediaInterfaceCode::FR1_100G,
            SMFMediaInterfaceCode::FR1_100G,
        },
        {
            // 2x100G-CWDM4
            SMFMediaInterfaceCode::CWDM4_100G,
            SMFMediaInterfaceCode::CWDM4_100G,
            SMFMediaInterfaceCode::CWDM4_100G,
            SMFMediaInterfaceCode::CWDM4_100G,
            SMFMediaInterfaceCode::CWDM4_100G,
            SMFMediaInterfaceCode::CWDM4_100G,
            SMFMediaInterfaceCode::CWDM4_100G,
            SMFMediaInterfaceCode::CWDM4_100G,
        },
        {
            // 1x800G-2FR4
            SMFMediaInterfaceCode::FR8_800G,
            SMFMediaInterfaceCode::FR8_800G,
            SMFMediaInterfaceCode::FR8_800G,
            SMFMediaInterfaceCode::FR8_800G,
            SMFMediaInterfaceCode::FR8_800G,
            SMFMediaInterfaceCode::FR8_800G,
            SMFMediaInterfaceCode::FR8_800G,
            SMFMediaInterfaceCode::FR8_800G,
        },
    };
    return smfOsfpValidSpeedCombinations_;
  }

  /*
   * getApplicationFromApSelCode
   *
   * Returns the interface code if ApSelCode is present in the module
   * capabilities.
   */
  template <typename InterfaceCode>
  static InterfaceCode getApplicationFromApSelCode(
      uint8_t apSelCode,
      const CmisModule::ApplicationAdvertisingFields& moduleCapabilities) {
    for (const auto& capability : moduleCapabilities) {
      if (capability.ApSelCode == apSelCode) {
        return static_cast<InterfaceCode>(capability.moduleMediaInterface);
      }
    }
    return InterfaceCode::UNKNOWN;
  }

  /*
   * getMediaIntfCodeFromSpeed
   *
   * Returns the media interface code for a given speed and the number of lanes
   * on this optics. This function uses the optics static value from register
   * cache. Pl note that the different optics can implement the same media
   * interface code using diferent number of lanes. ie: QSFP 400G-FR4 implements
   * 400G-FR4 (code 0x1d) using 8 lanes of 50G serdes on host whereas OSFP
   * 2x400G-FR4 implements the same 400G-FR4 (code 0x1d) using 4 lanes of 100G
   * serdes on host.
   */
  template <typename InterfaceCode>
  static InterfaceCode getMediaIntfCodeFromSpeed(
      cfg::PortSpeed speed,
      uint8_t numLanes,
      const CmisModule::ApplicationAdvertisingFields& moduleCapabilities) {
    auto appCodes = CmisHelper::getInterfaceCode(speed);
    if (appCodes.empty()) {
      return InterfaceCode::UNKNOWN;
    }

    for (auto application : appCodes) {
      for (const auto& capability : moduleCapabilities) {
        if (capability.moduleMediaInterface == application &&
            capability.hostLaneCount == numLanes) {
          return (InterfaceCode)application;
        }
      }
    }
    return InterfaceCode::UNKNOWN;
  }
};

} // namespace fboss
} // namespace facebook
