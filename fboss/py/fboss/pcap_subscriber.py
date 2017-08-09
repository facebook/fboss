from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import asyncio
import socket
import threading

from fboss.thrift_clients import PcapPushSubClient
from neteng.fboss.asyncio.pcap_pubsub import PcapSubscriber as ThriftSub
from thrift.server import TAsyncioServer


class PcapSubscriber(ThriftSub.Iface):

    def __init__(self, port):
        self.hostname = socket.gethostname()
        self.port = port

    def subscribe(self, pub_hostname):
        # setup client
        self._client = PcapPushSubClient(pub_hostname)
        self._client.subscribe(self.hostname, self.port)

    def unsubscribe(self):
        self._client.unsubscribe(self.hostname, self.port)

    # inherit this class and override the on receive functions
    # additionally, these functions need to be thread-safe


class PcapListener():

    def __init__(self, sub):
        self.subscriber = sub

    def thread_work(self):
        self.loop = asyncio.new_event_loop()
        self.server = self.loop.run_until_complete(
            TAsyncioServer.ThriftAsyncServerFactory(
                self.subscriber, port=self.subscriber.port, loop=self.loop
            )
        )
        self.subscriber.subscribe(self.remote)
        self.loop.run_forever()

    def open_connection(self, remote_host):
        self.remote = remote_host
        self.server_thread = threading.Thread(target=self.thread_work, args=())
        self.server_thread.daemon = True
        self.server_thread.start()
