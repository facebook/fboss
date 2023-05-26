// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/LldpManager.h"
#include "fboss/agent/Main.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/test/AgentTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <boost/container/flat_set.hpp>

DECLARE_string(oob_asset);
DECLARE_string(oob_flash_device_name);
DECLARE_string(openbmc_password);
DECLARE_bool(enable_lldp);
DECLARE_bool(tun_intf);
DECLARE_string(volatile_state_dir);
DECLARE_bool(setup_for_warmboot);
DECLARE_string(config);
DECLARE_bool(disable_neighbor_updates);

namespace facebook::fboss {

using namespace std::chrono_literals;

// When we switch to use qsfp_service to collect stats(PhyInfo), default stats
// collection frequency is 60s. Give the maximum check time 5x24=120s here.
constexpr auto kSecondsBetweenXphyInfoCollectionCheck = 5s;
constexpr auto kMaxNumXphyInfoCollectionCheck = 24;

class LinkTest : public AgentTest {
 protected:
  void SetUp() override;
  void overrideL2LearningConfig(bool swLearning = false, int ageTimer = 300);
  void waitForAllCabledPorts(
      bool up,
      uint32_t retries = 60,
      std::chrono::duration<uint32_t, std::milli> msBetweenRetry =
          std::chrono::milliseconds(1000)) const;
  void waitForAllTransceiverStates(
      bool up,
      uint32_t retries = 60,
      std::chrono::duration<uint32_t, std::milli> msBetweenRetry =
          std::chrono::milliseconds(1000)) const;
  std::map<int32_t, TransceiverInfo> waitForTransceiverInfo(
      std::vector<int32_t> transceiverIds,
      uint32_t retries = 2,
      std::chrono::duration<uint32_t, std::milli> msBetweenRetry =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::seconds(10))) const;
  bool checkReachabilityOnAllCabledPorts() const;
  /*
   * Get pairs of ports connected to each other
   */
  std::set<std::pair<PortID, PortID>> getConnectedPairs() const;

  /*
   * Return plugged in optical transceivers and their names.
   */
  std::tuple<std::vector<PortID>, std::string> getOpticalCabledPortsAndNames(
      bool pluggableOnly = false) const;

  /*
   * Ports where we expect optics to be plugged in.
   * In link tests this information is conveyed in config via non
   * null LLDP neighbors. We pick that up here to extract cabled ports
   */
  const std::vector<PortID>& getCabledPorts() const;
  const std::set<TransceiverID>& getCabledTranceivers() const {
    return cabledTransceivers_;
  }
  boost::container::flat_set<PortDescriptor> getVlanOwningCabledPorts() const;
  /*
   * Program default (v6) route over ports
   */

  void programDefaultRoute(
      const boost::container::flat_set<PortDescriptor>& ecmpPorts,
      std::optional<folly::MacAddress> dstMac = std::nullopt);
  /*
   * Disable TTL decrement on a set of ports
   */
  void disableTTLDecrements(
      const boost::container::flat_set<PortDescriptor>& ecmpPorts) const;
  /*
   * Create a L3 data plane loop and seed it with traffic
   */
  void createL3DataplaneFlood(
      const boost::container::flat_set<PortDescriptor>& inPorts);
  void createL3DataplaneFlood() {
    createL3DataplaneFlood(getVlanOwningCabledPorts());
  }
  PortID getPortID(const std::string& portName) const;
  std::string getPortName(PortID port) const;

  void waitForStateMachineState(
      const std::set<TransceiverID>& transceiversToCheck,
      TransceiverStateMachineState stateMachineState,
      uint32_t retries,
      std::chrono::duration<uint32_t, std::milli> msBetweenRetry) const;

  void waitForLldpOnCabledPorts(
      uint32_t retries = 60,
      std::chrono::duration<uint32_t, std::milli> msBetweenRetry =
          std::chrono::milliseconds(1000)) const;

  void setCmdLineFlagOverrides() const override;

  void restartQsfpService(bool coldboot) const;

  void TearDown() override;

  void setLinkState(bool enable, std::vector<PortID>& portIds);

 public:
  bool sendAndCheckReachabilityOnAllCabledPorts() {
    sw()->getLldpMgr()->sendLldpOnAllPorts();
    return checkReachabilityOnAllCabledPorts();
  }

 private:
  void programDefaultRoute(
      const boost::container::flat_set<PortDescriptor>& ecmpPorts,
      utility::EcmpSetupTargetedPorts6& ecmp6);
  void initializeCabledPorts();
  void logLinkDbgMessage(std::vector<PortID>& portIDs) const override;

  std::vector<PortID> cabledPorts_;
  std::set<TransceiverID> cabledTransceivers_;
};
int linkTestMain(
    int argc,
    char** argv,
    PlatformInitFn initPlatformFn,
    std::optional<cfg::StreamType> streamType = std::nullopt);
} // namespace facebook::fboss
