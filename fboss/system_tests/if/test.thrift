include "common/fb303/if/fb303.thrift"
include "fboss/agent/if/ctrl.thrift"
include "fboss/agent/if/fboss.thrift"

namespace py fboss.system_tests.test

const i32 DEFAULT_PORT = 20121

struct CapturedPacket {
  1: ctrl.fbbinary packet_data,
  /* Depending on how the packet is captured, len(packet_data) is not
     necessarily the same as packet_length,
     Ex: For jumbo frames snaplen option would be smaller than the packet size
  */
  2: i32 packet_length,
  // TODO(rsher): add timestamp info and other pcap metadata
}

enum DeviceType {
  LOOPBACK = 0
}

service TestService extends fb303.FacebookService {
  bool ping(1: string ip,
            2: optional list<string> options)
            throws (1: fboss.FbossBaseError error)
  i32 get_interface_mtu(1: string intf)
      throws (1: fboss.FbossBaseError error)
  string get_v4_ip(1: string intf)
      throws (1: fboss.FbossBaseError error)
  string get_v6_ip(1: string intf)
      throws (1: fboss.FbossBaseError error)
  bool status();

  /* This will start capturing packets, but the buffer is small so if lots
     of traffic is coming before you call startPktCapture(), you may miss
     the first few packets.  startPktCapture() will implicitly call
     stopPktCapture() on an interface that had been started but forgotten to
     stop.
   */
  void startPktCapture(1: string interfaceName, 2: string pcap_filter)
                       throws (1: fboss.FbossBaseError error)

  /* Will block until ms_timeout is reached or maxPackets are received */
  list<CapturedPacket> getPktCapture(1: string interfaceName,
                                     2: i32 ms_timeout,
                                     3: i32 maxPackets)
                                     throws (1: fboss.FbossBaseError error)
  /* It's good practice for a test to call this after a startPktCapture()
     to avoid wasting CPU resources.  The python test utils do this
     automatically, but if you don't use them and forget, it should not affect
     test correctness. */
  void stopPktCapture(1: string interfaceName) throws
                     (1: fboss.FbossBaseError error)

  /* Send a raw packet out this interface.  This is not optimized for
     performance, so don't use for line rate traffic generation */
  void sendPkt(1: string interfaceName, 2: ctrl.fbbinary pkt)
               throws (1: fboss.FbossBaseError error)

  /* initialize and listen to an iperf3 client test request. Returns a JSON
  formatted string which can be deserialized/loaded for post processing
  server-side test results.

  Useful links:
  =============
  * Iperf homepage:
  http://software.es.net/iperf/
  * Comparison of various iperf3 capabilities with other tools:
  https://fasterdata.es.net/performance-testing/network-troubleshooting-tools/throughput-tool-comparision/
   */

  string iperf3_server() throws (1: fboss.FbossBaseError error)

  /* initiate an iperf3 test to host server @ server_ip. server_ip can be
  ipv4 or ipv6. Returns a JSON formatted string which can be deserialized/loaded
  for post-processing of iperf3 client-side test results */

  string iperf3_client(1: string server_ip) throws
                      (1: fboss.FbossBaseError error)

  void flap_server_port(1: string interfaceName,
                       2: i32 duration,
                       3: i32 numberOfFlaps)
                       throws (1: fboss.FbossBaseError error)

  /* This will add an interface to the host.
     It will return True if succeeds, False if fails.
   */
  bool add_interface(1: string ifName, 2: DeviceType deviceType) throws
                      (1: fboss.FbossBaseError error)


  /* This will remove an interface to the host.
     It will return True if succeeds, False if fails.
   */
  bool remove_interface(1: string ifName) throws
                      (1: fboss.FbossBaseError error)

  /* This will add an address to an interface
     It will return True if succeeds, False if fails.
   */
  bool add_address(1: string address, 2: string ifName) throws
                      (1: fboss.FbossBaseError error)}
