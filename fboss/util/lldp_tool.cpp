// Copyright 2004-present Facebook. All Rights Reserved.
//
// This is a standalone utility that listens for LLDP and CDP packets,
// and prints information about each packet it sees.
//
// It can listen on both front-panel ports and local linux interfaces.
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/facebook/gen-cpp/bcm_config_types.h"
#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmRxPacket.h"
#include "fboss/agent/hw/bcm/BcmUnit.h"
#include "fboss/agent/lldp/LinkNeighbor.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/PktUtil.h"

#include <folly/Exception.h>
#include <folly/FileUtil.h>
#include <folly/MacAddress.h>
#include <folly/String.h>
#include <gflags/gflags.h>

#include <mutex>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/filter.h>
#include <linux/if_ether.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <time.h>

extern "C" {
#include <opennsl/l2.h>
#include <bcm/l2.h>
#include <opennsl/link.h>
#include <opennsl/port.h>
#include <opennsl/rx.h>
#include <opennsl/stg.h>
}

using namespace facebook::fboss;
using folly::checkUnixError;
using folly::humanify;
using folly::io::Cursor;
using folly::IOBuf;
using folly::MacAddress;
using folly::StringPiece;
using std::map;
using std::string;
using std::vector;

DEFINE_bool(bcm, true,
            "Whether to listen via a Broadcom ASIC.  "
            "Enabled by default; set to \"no\" to disable");
DEFINE_string(bcm_config, "",
              "The location of the Broadcom JSON configuration file");
DEFINE_int32(linkscan_interval_us, 250000,
             "The Broadcom linkscan interval");
DEFINE_string(if_name, "eth0", "The local interface to listen on");
DEFINE_int32(mtu, 9000, "The maximum packet size to expect");
DEFINE_bool(verbose, false,
             "Print more verbose information about each neighbor packet");

static const MacAddress MAC_LLDP_NEAREST_BRIDGE("01:80:c2:00:00:0e");
static const MacAddress MAC_CDP("01:00:0c:cc:cc:cc");

void initBcmAPI() {
  if (FLAGS_bcm_config.empty()) {
    // A BCM config file is required.
    LOG(ERROR) << "no --bcm_config argument specified";
    exit(1);
  }

  // Load from a local file
  string contents;
  if (!folly::readFile(FLAGS_bcm_config.c_str(), contents)) {
    throw FbossError("unable to read Broadcom config file ",
                     FLAGS_bcm_config);
  }
  bcm::BcmConfig cfg;
  cfg.readFromJson(contents.c_str());
  BcmAPI::init(cfg.config);
}

std::string formatNeighborInfoVerbose(LinkNeighbor* neighbor,
                                      StringPiece localPort) {
  std::string result;
  result.reserve(512);

  const char* protoStr =
    (neighbor->getProtocol() == LinkProtocol::LLDP) ? "LLDP" : "CDP";
  folly::toAppend(protoStr,
                  ": local_port=", localPort,
                  " vlan=", neighbor->getLocalVlan(),
                  "\n",
                  "  chassis_id[", neighbor->getChassisIdType(), "]=",
                  neighbor->humanReadableChassisId(),
                  "\n",
                  "  port_id[", neighbor->getPortIdType(), "]=",
                  neighbor->humanReadablePortId(),
                  "\n",
                  "  ttl=", neighbor->getTTL().count(),
                  "\n",
                  "  caps=", neighbor->getCapabilities(),
                  "\n",
                  "en_caps=", neighbor->getEnabledCapabilities(),
                  "\n",
                  &result);

  if (!neighbor->getSystemName().empty()) {
    folly::toAppend("  system_name=", humanify(neighbor->getSystemName()),
                    "\n",
                    &result);
  }

  if (!neighbor->getPortDescription().empty()) {
    folly::toAppend("  port_desc=", humanify(neighbor->getPortDescription()),
                    "\n",
                    &result);
  }

  if (!neighbor->getSystemDescription().empty()) {
    folly::toAppend("  system_desc=",
                    humanify(neighbor->getSystemDescription()), "\n",
                    &result);
  }

  return result;
}

std::string formatNeighborInfoSimple(LinkNeighbor* neighbor,
                                     StringPiece localPort) {
  std::string result;
  result.reserve(256);

  const char* protoStr =
    (neighbor->getProtocol() == LinkProtocol::LLDP) ? "LLDP" : "CDP";
  folly::toAppend(protoStr, ": local_port=", localPort, " remote_system=",
                  &result);
  // When available, the system_name field is generally better
  // than the chassis ID.
  if (neighbor->getSystemName().empty()) {
    result.append(neighbor->humanReadableChassisId());
  } else {
    result.append(neighbor->getSystemName());
  }

  folly::toAppend(" remote_port=", neighbor->humanReadablePortId(), "\n",
                  &result);
  return result;
}

void printNeighborInfo(LinkNeighbor* neighbor, StringPiece localPort) {
  std::string buf;
  if (FLAGS_verbose) {
    buf = formatNeighborInfoVerbose(neighbor, localPort);
  } else {
    buf = formatNeighborInfoSimple(neighbor, localPort);
  }

  // We may print from multiple threads simultaneously, so hold a mutex
  // while writing to ensure data from different threads doesn't get
  // interleaved.  We also just call write(), instead of using stdio,
  // since we want to make sure each entry is flushed as it arrives.
  static std::mutex stdoutMutex;
  std::lock_guard<std::mutex> g(stdoutMutex);
  write(STDOUT_FILENO, buf.data(), buf.size());
}

void processPacket(IOBuf* buf,
                   PortID srcPort,
                   VlanID srcVlan,
                   StringPiece localPort) {
  try {
    Cursor cursor(buf);
    MacAddress destMac = PktUtil::readMac(&cursor);
    MacAddress srcMac = PktUtil::readMac(&cursor);
    auto ethertype = cursor.readBE<uint16_t>();

    if (ethertype == ETHERTYPE_VLAN) {
      auto vlanTag = cursor.readBE<uint16_t>();
      ethertype = cursor.readBE<uint16_t>();
    }

    LinkNeighbor neighbor;
    bool parsed{false};
    if (ethertype == ETHERTYPE_LLDP) {
      parsed = neighbor.parseLldpPdu(srcPort, srcVlan, srcMac,
                                     ethertype, &cursor);
    } else if (destMac == MAC_CDP) {
      parsed = neighbor.parseCdpPdu(srcPort, srcVlan, srcMac,
                                    ethertype, &cursor);
    }

    if (parsed) {
      printNeighborInfo(&neighbor, localPort);
    } else {
      VLOG(3) << "ignoring non-LLDP packet";
    }
  } catch (const std::exception& ex) {
    LOG(ERROR) << "error parsing packet: " << folly::exceptionStr(ex);
  }
}

class BcmProcessor {
 public:
  BcmProcessor() {}
  virtual ~BcmProcessor() {}

  void prepare();

  /*
   * Start listening for packets.
   *
   * This starts the BCM RX thread, and returns immediately.
   */
  void start();

 private:
  void configurePort(opennsl_port_t port, opennsl_vlan_t vlan);

  static opennsl_rx_t lldpPktHandler(int unit, opennsl_pkt_t* nslPkt,
                                     void* cookie);

  int unit_{-1};
  std::unique_ptr<BcmUnit> bcmUnit_;
};

void BcmProcessor::prepare() {
  bcmUnit_ = BcmAPI::initOnlyUnit();
  bcmUnit_->attach();
  unit_ = bcmUnit_->getNumber();

  // Get the list of ports
  opennsl_port_config_t pcfg;
  auto rv = opennsl_port_config_get(unit_, &pcfg);
  bcmCheckError(rv, "failed to get port configuration");

  // Enable linkscan on all ports
  rv = opennsl_linkscan_mode_set_pbm(unit_, pcfg.port,
                                     OPENNSL_LINKSCAN_MODE_SW);
  bcmCheckError(rv, "failed to set linkscan ports");
  rv = opennsl_linkscan_enable_set(unit_, FLAGS_linkscan_interval_us);
  bcmCheckError(rv, "failed to enable linkscan");

  // Enable all ports, with each port in a separate VLAN.
  int idx;
  OPENNSL_PBMP_ITER(pcfg.port, idx) {
    opennsl_vlan_t vlan = 2 + idx;
    configurePort(idx, vlan);
  }
}

void BcmProcessor::start() {
  int rv;
  rv = opennsl_rx_register(0, "lldp_rx", lldpPktHandler, 1, this,
                           OPENNSL_RCO_F_ALL_COS);
  if (rv != 0) {
    LOG(ERROR) << "failed to register LLDP packet callback";
    return;
  }

  rv = opennsl_rx_start(0, nullptr);
  if (rv != 0) {
    LOG(ERROR) << "failed to start RX: " << opennsl_errmsg(rv);
    return;
  }
}

void BcmProcessor::configurePort(opennsl_port_t port, opennsl_vlan_t vlan) {
  VLOG(4) << "configuring port " << port;

  // Create a new VLAN for this port
  opennsl_pbmp_t pbmp;
  opennsl_pbmp_t ubmp;
  OPENNSL_PBMP_CLEAR(pbmp);
  OPENNSL_PBMP_PORT_ADD(pbmp, port);
  OPENNSL_PBMP_CLEAR(ubmp);
  OPENNSL_PBMP_PORT_ADD(ubmp, port);

  int rv = opennsl_vlan_create(unit_, vlan);
  bcmCheckError(rv, "failed to add VLAN ", vlan);
  rv = opennsl_vlan_port_add(unit_, vlan, pbmp, ubmp);
  bcmCheckError(rv, "failed to add members to new VLAN ", vlan);
  rv = opennsl_port_untagged_vlan_set(unit_, port, vlan);
  bcmCheckError(rv, "failed to set ingress VLAN for port ",
                port, " to ", vlan);

  // LLDP packets get trapped to the CPU by default, but we need to explicitly
  // request that packets for the CDP mac to be copied to the CPU.
  opennsl_l2_addr_t l2addr;
  opennsl_mac_t cdpMac;
  memcpy(&cdpMac, MAC_CDP.bytes(), MacAddress::SIZE);
  opennsl_l2_addr_t_init(&l2addr, cdpMac, vlan);
  l2addr.flags = OPENNSL_L2_STATIC | BCM_L2_COPY_TO_CPU;
  l2addr.port = port;
  rv = opennsl_l2_addr_add(unit_, &l2addr);
  bcmCheckError(rv, "failed to register for CDP packets on port ", port);

  // Set the spanning tree state to forwarding
  opennsl_stg_t stg = 1;
  rv = opennsl_stg_stp_set(unit_, stg, port, OPENNSL_STG_STP_FORWARD);
  bcmCheckError(rv, "failed to set spanning tree state on port ", port);

  // Enable the port
  int enable = 1;
  rv = opennsl_port_enable_set(unit_, port, enable);
  bcmCheckError(rv, "failed to enable port ", port);
}

opennsl_rx_t BcmProcessor::lldpPktHandler(int unit, opennsl_pkt_t* nslPkt,
                                          void* cookie) {
  // Ignore packets smaller than the minimum ethernet frame size
  int len = nslPkt->pkt_len;
  if (len < 64) {
    return OPENNSL_RX_NOT_HANDLED;
  }

  BcmRxPacket pkt(nslPkt);
  auto localPort = folly::to<string>(pkt.getSrcPort());
  processPacket(pkt.buf(), pkt.getSrcPort(), pkt.getSrcVlan(), localPort);
  return OPENNSL_RX_NOT_HANDLED;
}

class LocalInterfaceProcessor {
 public:
  explicit LocalInterfaceProcessor(folly::StringPiece interface)
    : interface_(interface.str()) {}

  virtual ~LocalInterfaceProcessor() {
    if (fd_ >= 0) {
      ::close(fd_);
    }
  }

  void prepare();

  /*
   * Listen for packets.
   *
   * This function runs forever and does not return.
   */
  void run();

 private:
  int fd_{-1};
  std::string interface_;
};

void LocalInterfaceProcessor::prepare() {
  fd_ = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  checkUnixError(fd_, "failed to create socket for local interface ",
                 interface_);

  // Look up the interface index
  struct ifreq ifr;
  snprintf(ifr.ifr_name, IFNAMSIZ, "%s", interface_.c_str());
  int rc = ioctl(fd_, SIOCGIFINDEX, &ifr);
  checkUnixError(rc, "failed to get interface index for ", interface_);

  // Bind the socket
  struct sockaddr_ll addr;
  addr.sll_family = AF_PACKET;
  addr.sll_ifindex = ifr.ifr_ifindex;
  addr.sll_protocol = htons(ETH_P_ALL);
  rc = bind(fd_, (struct sockaddr*)&addr, sizeof(addr));
  checkUnixError(rc, "failed to bind socket for ", interface_);

  // Ask linux to also send us packets sent to LLDP "nearest bridge" multicast
  // MAC address
  struct packet_mreq mr;
  memset(&mr, 0, sizeof(mr));
  mr.mr_ifindex = ifr.ifr_ifindex;
  mr.mr_alen = ETH_ALEN;
  memcpy(mr.mr_address, MAC_LLDP_NEAREST_BRIDGE.bytes(), ETH_ALEN);
  mr.mr_type = PACKET_MR_MULTICAST;
  rc = setsockopt(fd_, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr));
  checkUnixError(rc, "failed to add LLDP packet membership for ", interface_);

  // Ask linux to also send us packets sent to the CDP multicast MAC address
  memcpy(mr.mr_address, MAC_CDP.bytes(), ETH_ALEN);
  rc = setsockopt(fd_, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr));
  checkUnixError(rc, "failed to add CDP packet membership for ", interface_);

  // Set a filter to ignore non-multicast packets, so we don't get a flood
  // of meaningless packets that we don't care about.
  //
  // tcpdump -i eth0 -dd ether multicast
  static struct sock_filter PACKET_FILTER[] = {
    { 0x30, 0, 0, 0x00000000 },
    { 0x45, 0, 1, 0x00000001 },
    { 0x6, 0, 0, 0x0000ffff },
    { 0x6, 0, 0, 0x00000000 },
  };
  struct sock_fprog bpf;
  bpf.len = sizeof(PACKET_FILTER) / sizeof(PACKET_FILTER[0]);
  bpf.filter = PACKET_FILTER;
  rc = setsockopt(fd_, SOL_SOCKET, SO_ATTACH_FILTER, &bpf, sizeof(bpf));
  checkUnixError(rc, "failed to set socket packet filter for ", interface_);
}

void LocalInterfaceProcessor::run() {
  while (true) {
    IOBuf buf(IOBuf::CREATE, FLAGS_mtu);

    struct sockaddr_ll src_addr;
    socklen_t addr_len;
    ssize_t len = recvfrom(fd_, buf.writableTail(), buf.tailroom(), 0,
                           (struct sockaddr*)&src_addr, &addr_len);
    checkUnixError(len, "error reading packet from ", interface_);
    buf.append(len);

    PortID srcPort{0};
    VlanID srcVlan{0};
    processPacket(&buf, srcPort, srcVlan, interface_);
  }
}

int main(int argc, char* argv[]) {
  // Parse command line flags
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  google::SetCommandLineOptionWithMode("minloglevel", "0",
                                       google::SET_FLAGS_DEFAULT);
  google::InstallFailureSignalHandler();

  BcmProcessor bcm;
  if (FLAGS_bcm) {
    initBcmAPI();
    bcm.prepare();
  }

  LocalInterfaceProcessor local(FLAGS_if_name);
  local.prepare();

  printf("Listening for LLDP or CDP packets...\n");
  if (FLAGS_bcm) {
    bcm.start();
  }
  local.run();

  return 0;
}
