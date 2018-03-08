from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import json
from threading import Thread
from queue import Queue

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
        return {'server_resp':server_resp, 'client_resp':client_resp}

    # Update response as dict
    if server_ip:
        resp[server_ip] = _get_responses(server_ip)
    else:
        for src_ip in src_host.ips():
            resp[src_ip] = _get_responses(src_ip)
    return resp
