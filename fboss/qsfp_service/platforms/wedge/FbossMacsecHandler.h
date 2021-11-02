// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/mka_service/handlers/MacsecHandler.h"
#include "fboss/qsfp_service/TransceiverManager.h"

namespace facebook {
namespace fboss {

class FbossMacsecHandler : public mka::MacsecHandler {
 public:
  // Explicit constructor taking the wedgeManager object
  explicit FbossMacsecHandler(TransceiverManager* wedgeManager)
      : wedgeManager_(wedgeManager) {}

  // Virtual function for sakInstallRx. The Macsec supporting platform should
  // implement this API in the subclass
  virtual bool sakInstallRx(
      const mka::MKASak& /* sak */,
      const mka::MKASci& /* sci */) override {
    return false;
  }

  // Virtual function for sakInstallTx. The Macsec supporting platform should
  // implement this API in the subclass
  virtual bool sakInstallTx(const mka::MKASak& /* sak */) override {
    return false;
  }

  // Virtual function for sakDeleteRx. The Macsec supporting platform should
  // implement this API in the subclass
  virtual bool sakDeleteRx(
      const mka::MKASak& /* sak */,
      const mka::MKASci& /* sci */) override {
    return false;
  }

  // Virtual function for sakDelete. The Macsec supporting platform should
  // implement this API in the subclass
  virtual bool sakDelete(const mka::MKASak& /* sak */) override {
    return false;
  }

  // Virtual function for sakHealthCheck. The Macsec supporting platform should
  // implement this API in the subclass
  virtual mka::MKASakHealthResponse sakHealthCheck(
      const mka::MKASak& /* sak */) override {
    mka::MKASakHealthResponse result;
    return result;
  }

  // Virtual function to get the phy port link info
  virtual PortOperState macsecGetPhyLinkInfo(
      const std::string& /* portName */) override {
    return PortOperState::DOWN;
  }

  // Virtual function to get the phy port link info
  virtual bool deleteAllSc(const std::string& /* portName */) override {
    return false;
  }

  // Virtual function to get the Port stats
  virtual mka::MacsecPortStats macsecGetPortStats(
      const std::string& /* portName */,
      bool /* directionIngress */,
      bool /*readFromHw*/) override {
    return mka::MacsecPortStats();
  }

  // Virtual function to get the flow stats
  virtual mka::MacsecFlowStats macsecGetFlowStats(
      const std::string& /* portName */,
      bool /* directionIngress */,
      bool /*readFromHw*/) override {
    return mka::MacsecFlowStats();
  }

  // Virtual function to get the SA stats
  virtual mka::MacsecSaStats macsecGetSaStats(
      const std::string& /* portName */,
      bool /* directionIngress */,
      bool /*readFromHw*/) override {
    return mka::MacsecSaStats();
  }

  virtual std::map<std::string, MacsecStats> getAllMacsecPortStats() override {
    return std::map<std::string, MacsecStats>{};
  }

  virtual std::map<std::string, MacsecStats> getMacsecPortStats(
      const std::vector<std::string>& /* portName */) override {
    return std::map<std::string, MacsecStats>{};
  }

 protected:
  // TransceiverManager or WedgeManager object pointer passed by QsfpService
  // main
  TransceiverManager* wedgeManager_;
};

} // namespace fboss
} // namespace facebook
