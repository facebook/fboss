from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

"""
A service to run on testing hosts, so that we can source traffic, sync
traffic, etc.

Modeled after pyserver example in open source
https://github.com/facebook/fbthrift/blob/master/thrift/tutorial/\
                                                       py/PythonServer.py
"""
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import THeaderProtocol
from thrift.server import TServer

from neteng.fboss.ttypes import FbossBaseError
from fboss.system_tests.test import TestService
from fboss.system_tests.test.constants import DEFAULT_PORT

import logging
import pcapy            # this library currently only supports python 2.x :-(
import subprocess
import time
import traceback


class TestServer(TestService.Iface):
    SERVICE = TestService
    PCAP_READ_BATCHING = 100  # in miliseconds

    def __init__(self):
        self.log = logging.getLogger(self.__class__.__name__)
        self.log.addHandler(logging.StreamHandler())
        self.log.setLevel(logging.INFO)
        self.pcap_captures = {}
        self.pkt_captures = {}
        self.log.debug("Log: debug enabled")
        self.log.info("Log: info enabled")
        self.log.warn("Log: warnings enabled")

    def ping(self, ip):
        """ @param ip : a string, e.g., "128.8.128.118" """
        options = ['-c 1']
        if ":" in ip:
            options.append('-6')
        # Using subprocess.call with shell=False should prevent
        # any security concerns because this just calls exec() natively
        # (assuming there are no commandline buffer overflows in ping)
        command = ["ping"] + options + [ip]
        self.log.info("Ping: running `%s`" % " ".join(command))
        with open("/dev/null") as devnull:
            response = subprocess.call(command, stdout=devnull)
        return response == 0

    def status(self):
        return True

    def startPktCapture(self, interface_name, pcap_filter_str):
        self.log.info("startPktCapture(%s,filter=%s)" % (
                      interface_name, pcap_filter_str))
        if interface_name in self.pcap_captures:
            # close out any old captures
            del self.pcap_captures[interface_name]
        reader = pcapy.open_live(interface_name, 1500,
                                 1, self.PCAP_READ_BATCHING)
        reader.setnonblock(1)
        reader.setfilter(pcap_filter_str)
        self.pcap_captures[interface_name] = reader
        self.pkt_captures[interface_name] = []

    def _pkt_callback(self, interface_name, pkt_hdr, pkt_data):
        pkts = self.pkt_captures[interface_name]
        self.log.debug("_pkt_callback(%s): %d pkts captured" %
                       (interface_name, len(pkts)))
        pkts.append((pkt_hdr, pkt_data))

    def getPktCapture(self, interface_name, ms_timeout, maxPackets):
        """ All documentation for this call is in thrift definition """
        if interface_name not in self.pcap_captures:
            raise FbossBaseError("No startPktCapture() for Interface " +
                                 interface_name)
        reader = self.pcap_captures[interface_name]
        start = time.time()
        intf = interface_name  # horrible hack to fit in 80 chars below
    #    pdb.set_trace()
        self.log.info("getPktCapture(%s,ms_timeout=%d,maxPackets=%d)" %
                      (interface_name, ms_timeout, maxPackets))
        while time.time() < (start + (ms_timeout / 1000)):
            # this reader is set to not block, so this will busy wait until
            # packets show up or the timeout occurs.  Given the note
            # about the readtimeout param in open_live() not being widely
            # supported, this is the best we can do
            try:
                reader.dispatch(
                    maxPackets,
                    lambda phdr, pdata: self._pkt_callback(intf, phdr, pdata))
            except Exception as e:
                traceback.print_exc()
                print(str(e))
                raise
            pkts = self.pkt_captures[interface_name]
            self.log.debug("    got %d packets" % len(pkts))
            if len(pkts) >= maxPackets:
                break
        return_pkts = []
        for _phdr, pdata in pkts:
            capture = TestService.CapturedPacket()
            capture.packet_data = pdata
            return_pkts.append(capture)
        return return_pkts

    def stopPktCapture(self, interface_name):
        if interface_name not in self.pcap_captures:
            raise FbossBaseError("Calling stopPktCapture() without " +
                                 " startPktCapture()?")
        del self.pcap_captures[interface_name]

    def sendPkt(self, interface_name, pkt):
        """ To send a packet, we need an open_live call.
            If one's already open, use that, if not, start and
            stop one quickly """
        need_cleanup = False
        if interface_name not in self.pcap_captures:
            need_cleanup = True
            self.startPktCapture(interface_name, "")
        self.pcap_captures[interface_name].sendpacket(pkt)
        if need_cleanup:
            self.stopPktCapture(interface_name)


if __name__ == '__main__':
    handler = TestServer()
    transport = TSocket.TServerSocket(DEFAULT_PORT)
    tfactory = TTransport.TBufferedTransportFactory()
    pfactory = THeaderProtocol.THeaderProtocolFactory()

    server = TServer.TSimpleServer(handler, transport, tfactory, pfactory)

    # You could do one of these for a multithreaded server
    # server = TServer.TThreadedServer(handler, transport, tfactory, pfactory)

    print('Starting the server...')
    server.serve()
    print('done.')
