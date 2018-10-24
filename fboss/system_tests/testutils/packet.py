from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import ipaddress
import json
from threading import Thread
from queue import Queue
import scapy.all as scapy
import time
from fboss.system_tests.testutils.ip_conversion import ip_addr_to_str
from fboss.system_tests.testutils.fb303_utils import get_fb303_counter


DOWNLINK_VLAN = 2000
# Due to SDK stats thread (1/sec) and Fboss stats thread (1/sec)
# it's possible that it takes a full two seconds in the worst case
# for a fb303 counter to increment after the packet passes
# also add time for scheduling ... just make it big
MAX_COUNTER_DELAY = 10  # in seconds


def make_packet(src_host, dst_host, ttl=64):
    pass


def is_v4(addr):
    """
    @param addr: a string or anything ipaddress lib can parse
    """
    return isinstance(ipaddress.ip_address(addr), ipaddress.IPv4Address)


def is_v6(addr):
    """
    @param addr: a string or anything ipaddress lib can parse
    """
    return isinstance(ipaddress.ip_address(addr), ipaddress.IPv6Address)


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
        dst_port=54321):
    # are we using v6?   Assume yes until told otherwise
    v6 = True if dst_ip is None else is_v6(dst_ip)
    if src_ip is None:
        src_ip = 'fe80:1:2:3:4::1' if v6 else '1.2.3.4'
    else:
        v6 = is_v6(src_ip)

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
    if v6 or ':' in dst_ip:
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


def _wait_for_counter_to_stop(test, switch_thrift, counter, delay,
        max_queue_time=2.5):
    # wait for counter to stop incrementing,
    # so no old/queued packets corrupt our test data
    # 2.5 is a bit of a mgaic number because of the current
    # counter bump queuing delay of 2 seconcs as described at the
    # top of this file
    npkts_start = get_fb303_counter(test, counter)
    start = time.time()
    while (start + delay) > time.time():
        # assume no packets queued for more than max_queue seconds
        time.sleep(max_queue_time)
        npkts_before = get_fb303_counter(test, counter)
        if npkts_before != npkts_start:
            # need to wait longer
            npkts_start = npkts_before
        else:
            break
    test.assertEqual(npkts_before, npkts_start,
                        "Packets never stopped; test broken!?")
    return npkts_before


def _wait_for_pkts_to_be_counted(test, switch_thrift, counter, sent_pkts,
                                 delay, npkts_before):
        # wait for counter to increment at least sent_pkts times before delay
        start = time.time()
        found = False
        while not found and (start + delay) > time.time():
            npkts_after = get_fb303_counter(test, counter)
            if npkts_after >= npkts_before + sent_pkts:
                found = True
            else:
                time.sleep(0.5)  # sleep a little intead of busy wait
        delta = time.time() - start
        # TODO log to scuba and track time delta
        # to expose/track stats thread lag/latency -- seems to be 1-5 seconds!?
        test.log.info("Packets after: %d secs %f" % (npkts_after, delta))
        return npkts_after


def _verify_counter_bump(test, counter, sent_pkts, received):
        test.log.info("Packets: got %d; wanted %d" % (received, sent_pkts))
        # first, make sure something hit the counter
        test.assertGreater(received, 0,
                    "No packets hit the counter %s !?" % counter)
        # second, make sure all of the packets sent were counted
        test.assertGreaterEqual(received, sent_pkts,
                    "All packets didn't hit the counter %s!?" % counter)
        # third, make sure the counter isn't just going crazy
        # this test is a bit imprecise and may not have real value
        # TODO{rsher}: figure out why this test fails infrequently
        # - currently 1:254 times; make a warning for now
        if (received > (2 * sent_pkts)):
            test.log.warning("Received more than 2x expected pkts on %s!?"
                    % counter)


def send_pkt_verify_counter_bump(test,  # an instance of FbossBaseSystemTest
                                 pkt,   # array of bytes
                                 counter,  # string of fb303 counter name
                                 **kwargs):
        return send_pkts_verify_counter_bump(test, [pkt], counter, **kwargs)


def send_pkts_verify_counter_bump(test,  # an instance of FbossBaseSystemTest
                                  pkts,   # array of array of bytes
                                  counter,  # string of fb303 counter name
                                  delay=MAX_COUNTER_DELAY,  # time in seconds
                                  min_packets=100):
    " Does sending this packet increment this counter?"
    test.test_topology.min_hosts_or_skip(1)  # need >1 host to run this
    with test.test_topology.switch_thrift() as switch_thrift:
        # get the current count of packets
        npkts_before = _wait_for_counter_to_stop(test, switch_thrift, counter,
                                                 delay)
        host = test.test_topology.hosts()[0]  # guaranteed to have 1
        # send packets that are supposed to hit the queue/counter
        with host.thrift_client() as client:
            intf = host.intfs()[0]
            test.log.info('sending packet from host %s intf=%s',
                          host, intf)
            sent_pkts = 0
            while sent_pkts < min_packets:
                for pkt in pkts:
                    client.sendPkt(intf, pkt)
                sent_pkts += len(pkts)
        # get the new count of packets
        npkts_after = _wait_for_pkts_to_be_counted(test, switch_thrift, counter,
                                                    sent_pkts, delay,
                                                    npkts_before)
        _verify_counter_bump(test, counter, sent_pkts,
                                npkts_after - npkts_before)
