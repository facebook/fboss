from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import json
from threading import Thread
from queue import Queue
import scapy.all as scapy
import time
from fboss.system_tests.testutils.ip_conversion import ip_addr_to_str


DOWNLINK_VLAN = 2000
# Due to SDK stats thread (1/sec) and Fboss stats thread (1/sec)
# it's possible that it takes a full two seconds in the worst case
# for a fb303 counter to increment after the packet passes
MAX_COUNTER_DELAY = 2  # in seconds


def make_packet(src_host, dst_host, ttl=64):
    pass


def run_iperf(src_host, dst_host, server_ip=None, duration=10, options=None):
    '''
    Inputs
    1. src_host and dst_host of type TestHost
    2. Optional:
        a. server_ip - Server IP on which Iperf client connects to
        b. duration for which server would be running
        c. default server options - '-J -1 -s', For additional flags
           update options
    Returns:
    {
        'server_ip1':{'server_resp':serv_resp, 'client_resp':client_resp},
        'server_ip2':{'server_resp':serv_resp, 'client_resp':client_resp},
    }
    '''

    resp = {}
    # Run Iperf and return responses from both src/dst as dict

    def _get_responses(source_ip):
        with src_host.thrift_client() as iperf_server:
            que = Queue()
            srv_thread = Thread(target=lambda q:
                                q.put(iperf_server.iperf3_server(timeout=duration,
                                                                 options=options)),
                                                                 args=(que, ))
            srv_thread.start()
            with dst_host.thrift_client() as iperf_client:
                client_resp = iperf_client.iperf3_client(server_ip)
                client_resp = json.loads(client_resp)
                srv_thread.join()
                server_resp = json.loads(que.get())
        return {'server_resp': server_resp, 'client_resp': client_resp}

    # Update response as dict
    if server_ip:
        resp[server_ip] = _get_responses(server_ip)
    else:
        for src_ip in src_host.ips():
            resp[src_ip] = _get_responses(src_ip)
    return resp


def gen_pkt_to_switch(test,  # something that inherits from FbossBaseSystemTest
        src_eth="00:11:22:33:44:55",
        dst_eth=None,  # fill in from switch
        src_ip=None,   # fill in based on v6=True
        dst_ip=None,   # fill in with switch's inband interface
        src_port=12345,
        dst_port=54321,
        v6=True):
    if src_ip is None:
        src_ip = 'fe80:1:2:3:4::1' if v6 else '1.2.3.4'
    if dst_ip is None or dst_eth is None:
        with test.test_topology.switch_thrift() as client:
            interfaces = client.getAllInterfaces()
        test.assertGreater(len(interfaces), 0, "No interfaces found!?")
        if DOWNLINK_VLAN in interfaces:
            interface = interfaces[DOWNLINK_VLAN]
        else:
            # grab an interface, doesn't matter which one because
            # they all share the same MAC (currently)
            interface = next(iter(interfaces.values()))
        if dst_eth is None:
            dst_eth = interface.mac
        if dst_ip is None:
            dst_ip = _get_first_router_ip(test, interface, v6)
    frame = scapy.Ether(src=src_eth, dst=dst_eth)
    if v6:
        pkt = scapy.IPv6(src=src_ip, dst=dst_ip)
    else:
        pkt = scapy.IP(src=src_ip, dst=dst_ip)
    segment = scapy.TCP(sport=src_port, dport=dst_port)
    pkt = frame / pkt / segment
    test.log.info("Creating packet %s" % repr(pkt))
    return bytes(pkt)  # return raw packet in bytes


def _get_first_router_ip(test, interface, v6):
    addr_len = 16 if v6 else 4
    # just grab any IP as long as it's the right type
    dst_ips = list(filter(lambda prefix: len(prefix.ip.addr) == addr_len,
                            interface.address))
    test.assertGreaterEqual(len(dst_ips), 1,
            "Switch has no %s route IPs" % "v6" if v6 else "v4")
    return ip_addr_to_str(dst_ips[0].ip)


def send_pkt_verify_counter_bump(test,  # an instance of FbossBaseSystemTest
                                 pkt,   # array of bytes
                                 counter,  # string of fb303 counter name
                                 delay=MAX_COUNTER_DELAY):  # time in seconds
    " Does sending this packet increment this counter?"
    test.test_topology.min_hosts_or_skip(1)  # need >1 host to run this
    with test.test_topology.switch_thrift() as switch_thrift:
        # get the current count of packets
        npkts_before = switch_thrift.getCounter(counter)
        host = test.test_topology.hosts()[0]  # guaranteed to have 1
        # send a packet that's supposed to hit the queue/counter
        with host.thrift_client() as client:
            intf = host.intfs()[0]
            test.log.info('sending packet from host %s intf=%s',
                          host, intf)
            client.sendPkt(intf, pkt)
        # sleep  because the stats counting thread takes time to schedule
        time.sleep(delay)
        # get the new count of packets
        npkts_after = switch_thrift.getCounter(counter)
        # make sure we got at least one more
        # note that if there is stray traffic on the link, this can
        # cause false positives.  See FBCoppTest for Facebook specific
        # tests that prevent this problem.
        test.assertGreater(npkts_after, npkts_before,
                           "Packet didn't hit the counter %s!?" % counter)
