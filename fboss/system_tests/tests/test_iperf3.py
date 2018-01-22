from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import json
from threading import Thread
from queue import Queue

from fboss.system_tests.system_tests import FbossBaseSystemTest, test_tags


@test_tags("iperf", "run-on-diff")
class Iperf3AllPairs(FbossBaseSystemTest):
    """ Make sure each host can ping every other host """
    def test_pair_iperf3(self):
        for src_host in self.test_topology.hosts():
            for dst_host in self.test_topology.hosts():
                if src_host == dst_host:
                    continue    # don't bother pinging ourselves
                with src_host.thrift_client() as iperf_server:
                    for server_ip in src_host.ips():
                        self.log.info("iperf3 server up at %s" % server_ip)
                        que = Queue()
                        srv_thread = Thread(target=lambda q:
                                            q.put(iperf_server.iperf3_server()),
                                            args=(que, ))
                        srv_thread.start()
                        with dst_host.thrift_client() as iperf_client:
                            client_resp = iperf_client.iperf3_client(server_ip)
                            client_resp = json.loads(client_resp)
                            srv_thread.join()
                            server_resp = json.loads(que.get())
                            self.log.info('client result: %s' % client_resp)
                            self.log.info('server result: %s' % server_resp)
                            self.assertTrue(isinstance(server_resp, dict))
                            self.assertTrue(isinstance(client_resp, dict))
                            self.assertFalse('error' in server_resp.keys())
                            self.assertFalse('error' in client_resp.keys())
                            self.assertTrue(server_ip == client_resp['start']
                                            ['connecting_to']['host'])
