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
from fboss.system_tests.test.ttypes import DeviceType
from fboss.system_tests.test.constants import DEFAULT_PORT
from os.path import isfile

import json
import logging
import pcapy            # this library currently only supports python 2.x :-(
import signal
import socket
import subprocess
import time
import traceback


class TestServer(TestService.Iface):
    SERVICE = TestService
    PCAP_READ_BATCHING = 100  # in miliseconds
    DEVICE_TYPE_ENUM_TO_STRING = {
        DeviceType.LOOPBACK: "dummy"
    }

    def __init__(self):
        self.log = logging.getLogger(self.__class__.__name__)
        self.log.addHandler(logging.StreamHandler())
        self.log.setLevel(logging.INFO)
        self.pcap_captures = {}
        self.pkt_captures = {}
        self.log.debug("Log: debug enabled")
        # Keep track of added_interfaces so users can't remove interfaces
        # they didn't add
        self.added_interfaces = set()
        signal.signal(signal.SIGINT, self._cleanUpAddedInterfaces)
        self.log.info("Log: info enabled")
        self.log.warn("Log: warnings enabled")

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self._cleanUpAddedInterfaces()

    def ping(self, ip, options=None):
        """ @param ip : a string, e.g., "128.8.128.118" """
        # Ping default options is to capture 1 packet
        if not options:
            options = ['-c', '1']
        if ":" in ip:
            options.append('-6')

        # Using subprocess.call with shell=False should prevent
        # any security concerns because this just calls exec() natively
        # (assuming there are no commandline buffer overflows in ping)
        command = ["ping"] + options + [ip]
        self.log.debug("Ping: running `%s`" % " ".join(command))
        with open("/dev/null") as devnull:
            response = subprocess.call(command, stdout=devnull)
        return response == 0

    def get_interface_mtu(self, intf):
        command = ["cat", "/sys/class/net/%s/mtu" % intf]
        response = subprocess.check_output(command)
        return int(response)

    def get_v4_ip(self, intf):
        command = ["ip", "-4", "addr", "show", intf]
        response = subprocess.check_output(command)
        return response

    def get_v6_ip(self, intf):
        command = ["ip", "-6", "addr", "show", intf]
        response = subprocess.check_output(command)
        return response

    def status(self):
        return True

    def startPktCapture(self, interface_name, pcap_filter_str):
        self.log.debug("startPktCapture(%s,filter=%s)" % (
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
        self.log.debug("getPktCapture(%s,ms_timeout=%d,maxPackets=%d)" %
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
            capture.packet_length = _phdr.getlen()
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
        self.log.warning("Doing sendPkt(%s,len(%s))" % (interface_name,
                                                    len(pkt)))
        ## NOTE: pcapy.reader.sendpacket() is not implemented!  use python
        raw = socket.socket(socket.AF_PACKET, socket.SOCK_RAW)
        raw.bind((interface_name, 0))
        raw.send(pkt)

    @staticmethod
    def check_output(cmd, **kwargs):
        return subprocess.check_output(cmd.split(' '), **kwargs)

    def kill_iperf3(self):
        try:
            self.check_output('pkill -9 iperf3')
        except Exception:
            pass

    def iperf3_server(self, timeout=10):
        ''' initialize iperf3 server, with timeout to expire server if no
        client request comes in
        '''
        def timeout_handler(signum, frame):
            raise FbossBaseError("IPERF3 SERVER TIMEOUT")

        self.kill_iperf3()  # kill lingering iperf3 processes (server or client)
        signal.signal(signal.SIGALRM, timeout_handler)
        signal.alarm(timeout)
        iperf3_pid_fn = '/tmp/iperf3_thrift.pid'
        command = "iperf3 -I {fn} -J -1 -s".format(fn=iperf3_pid_fn)
        try:
            response = self.check_output(command)
        except Exception as e:
            response = json.dumps({'error': repr(e)})
        finally:
            signal.alarm(0)
            if isfile(iperf3_pid_fn):
                with open(iperf3_pid_fn, 'r') as f:
                    pid = f.read().strip('\0')
                self.check_output('kill -9 {pid}'.format(pid=pid))
        return response

    def iperf3_client(self, server_ip, client_retries=3):
        ''' @param ip : a string, e.g., "128.8.128.118"
            @param client_retries: int, how many retries client attempts
        '''

        self.kill_iperf3()  # kill lingering iperf3 processes (server or client)
        is_ipv6 = '-6' if ':' in server_ip else ''
        command = "iperf3 {} -J -t 1 -c {}".format(is_ipv6, server_ip)
        client_loop_cnt = 0
        while client_loop_cnt < client_retries:
            try:
                response = self.check_output(command)
                break
            except Exception:
                client_loop_cnt += 1
                error_msg = '{} retries to reach iperf3 server {}' \
                            .format(client_loop_cnt, server_ip)
                response = json.dumps({'error': error_msg})
                time.sleep(1)
        return response

    def flap_server_port(self, interface, numberOfFlaps, sleepDuration):
        ''' System test to verify port flap handling works and doesn't hang
            the system.
        '''
        command_for_interface_down = 'ifconfig ' + interface + " down"
        command_for_interface_up = 'ifup ' + interface

        for _iteration in range(1, numberOfFlaps + 1):
            # Ignore the SIGHUP signal to maintain session when the server port
            # is turned down.
            old_handler = signal.signal(signal.SIGHUP, signal.SIG_IGN)

            self.log.debug("Flap iteration {}".format(_iteration))

            self.check_output(command_for_interface_down)
            time.sleep(sleepDuration)

            self.check_output(command_for_interface_up)

            # Change the SIGHUP settings back to original value.
            signal.signal(signal.SIGHUP, old_handler)

    def add_interface(self, ifName, deviceType):
        if ifName in self.added_interfaces:
            raise FbossBaseError(
                "Device {} already exists".format(ifName))

        strDeviceType = self.DEVICE_TYPE_ENUM_TO_STRING.get(deviceType)
        if not strDeviceType:
            raise FbossBaseError(
                "DeviceType {} not found/supported (are you sure you used the enum?)"
                .format(deviceType))

        command = "ip link add name {} type {}".format(ifName, strDeviceType)
        try:
            self.check_output(command)
        except Exception as e:
            # Ignoring "File exists" error
            if "exit status 2" not in str(e):
                raise FbossBaseError("Error adding interface: {}".format(str(e)))
        self.added_interfaces.add(ifName)
        return True

    def remove_interface(self, ifName):
        if ifName not in self.added_interfaces:
            raise FbossBaseError(
                "User attempted to remove an interface they did not add. Ignoring.")

        command = "ip link del dev {}".format(ifName)
        try:
            self.check_output(command)
        except Exception as e:
            raise FbossBaseError("Error deleting interface: {}".format(str(e)))
        self.added_interfaces.remove(ifName)
        return True

    def add_address(self, address, ifName):
        command = "ip addr add {} dev {}".format(address, ifName)
        try:
            self.check_output(command)
        except Exception as e:
            # Ignoring "File exists" error
            if "exit status 2" not in str(e):
                raise FbossBaseError("Error adding address: {}".format(str(e)))
        return True

    def _cleanUpAddedInterfaces(self):
        for ifName in self.added_interfaces:
            command = "ip link del dev {}".format(ifName)
            try:
                self.check_output(command)
            except Exception as e:
                self.log.info("Error deleting interface: {}".format(str(e)))


if __name__ == '__main__':
    with TestServer() as handler:
        transport = TSocket.TServerSocket(DEFAULT_PORT)
        tfactory = TTransport.TBufferedTransportFactory()
        pfactory = THeaderProtocol.THeaderProtocolFactory()

        server = TServer.TSimpleServer(handler, transport, tfactory, pfactory)

        # You could do one of these for a multithreaded server
        # server = TServer.TThreadedServer(handler, transport, tfactory, pfactory)

        print('Starting the server...')
        server.serve()
        print('Done.')
