// Copyright 2004-present Facebook.  All rights reserved.
#pragma once

#include <fboss/mka_service/if/gen-cpp2/mka_structs_types.h>
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

namespace facebook {
namespace fboss {
namespace mka {

// Handler that configures the macsec interfaces and other macsec options
class MacsecHandler {
 public:
  MacsecHandler() = default;
  virtual ~MacsecHandler() = default;

  /**
   * Install SAK in Rx
   **/
  virtual bool sakInstallRx(const MKASak& sak, const MKASci& sci) = 0;

  /**
   * Install SAK in Tx
   **/
  virtual bool sakInstallTx(const MKASak& sak) = 0;

  /**
   * Delete SAK in Rx
   **/
  virtual bool sakDeleteRx(const MKASak& sak, const MKASci& sci) = 0;

  /**
   * Clear macsec on this interface.
   */
  virtual bool sakDelete(const MKASak& sak) = 0;

  /*
   * Check if the interface is healthy. If the return value is false,
   * then re-negotiation will start.
   */

  virtual MKASakHealthResponse sakHealthCheck(const MKASak& sak) = 0;

  /*
   * Get the Phy port map from Macsec handler
   */
  virtual MacsecPortPhyMap macsecGetPhyPortInfo(
      const std::vector<std::string>& /* portNames */) {
    return MacsecPortPhyMap{};
  }

  /*
   * Get the Phy port link information from Macsec handler
   */
  virtual PortOperState macsecGetPhyLinkInfo(
      const std::string& /* portName */) {
    return PortOperState::DOWN;
  }

  /*
   * Get the Phy port link information from Macsec handler
   */
  virtual phy::PhyInfo getPhyInfo(const std::string& /* portName */) {
    phy::PhyInfo phyInfo;
    phyInfo.phyChip().ensure();
    phyInfo.line().ensure();
    return phyInfo;
  }

  /*
   * Delete all SA, SC and related Acl/Flow entries from a Phy
   */
  virtual bool deleteAllSc(const std::string& /* portName */) {
    return true;
  }

  /*
   * Set the Macsec state for port as per the fields provided
   */
  virtual bool setupMacsecState(
      const std::vector<std::string>& /* portList */,
      bool /* macsecDesired */,
      bool /* dropUnencrypted */) {
    return true;
  }

  /*
   * Get all the macsec port stats
   */
  virtual std::map<std::string, MacsecStats> getAllMacsecPortStats(
      bool /* readFromHw */) {
    return std::map<std::string, MacsecStats>{};
  }

  /*
   * Get a macsec port stats
   */
  virtual std::map<std::string, MacsecStats> getMacsecPortStats(
      const std::vector<std::string>& /* portNames */,
      bool /* readFromHw */) {
    return std::map<std::string, MacsecStats>{};
  }

 private:
  MacsecHandler(const MacsecHandler&) = delete;
  MacsecHandler& operator=(const MacsecHandler&) = delete;
};

} // namespace mka
} // namespace fboss
} // namespace facebook
