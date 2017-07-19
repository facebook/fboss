include "common/fb303/if/fb303.thrift"
include "fboss/agent/if/ctrl.thrift"
include "fboss/agent/if/fboss.thrift"

namespace py fboss.system_tests.test

const i32 DEFAULT_PORT = 20121

struct CapturedPacket {
  1: ctrl.fbbinary packet_data,
  // TODO(rsher): add timestamp info and other pcap metadata
}

service TestService extends fb303.FacebookService {
  bool ping(1: string ip) throws (1: fboss.FbossBaseError error)
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
}
