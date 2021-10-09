// Copyright 2004-present Facebook.  All rights reserved.
#pragma once

#include <fboss/mka_service/if/gen-cpp2/mka_structs_types.h>
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

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
  virtual MacsecPortPhyMap macsecGetPhyPortInfo() {
    return MacsecPortPhyMap{};
  }

  /*
   * Get the Phy port link information from Macsec handler
   */
  virtual PortOperState macsecGetPhyLinkInfo(const std::string& /* portName */) {
    return PortOperState::DOWN;
  }

  /*
   * Delete all SA, SC and related Acl/Flow entries from a Phy
   */
  virtual bool deleteAllSc(const std::string& /* portName */) {
    return true;
  }

  /*
   * Get the list of all SC SA on a phy
   */
  virtual MacsecAllScInfo
  macsecGetAllScInfo(const std::string& /* portName */) {
    return MacsecAllScInfo{};
  }

  /*
   * Get the macsec Sa stats
   */
  virtual MacsecPortStats macsecGetPortStats(
      const std::string& /* portName */,
      bool /* directionIngress */,
      bool /* readFromHw */) {
    return MacsecPortStats{};
  }

  /*
   * Get the macsec Flow stats
   */
  virtual MacsecFlowStats macsecGetFlowStats(
    const std::string& /* portName */,
    bool /* directionIngress */,
    bool /* readFromHw */) {
    return MacsecFlowStats{};
  }

  /*
   * Get the macsec Sa stats
   */
  virtual MacsecSaStats macsecGetSaStats(
      const std::string& /* portName */,
    bool /* directionIngress */,
    bool /* readFromHw */) {
    return MacsecSaStats{};
  }

 private:
  MacsecHandler(const MacsecHandler&) = delete;
  MacsecHandler& operator=(const MacsecHandler&) = delete;
};

} // namespace mka
} // namespace fboss
} // namespace facebook
