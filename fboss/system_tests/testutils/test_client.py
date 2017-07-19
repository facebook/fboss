from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import socket
import sys

from fboss.system_tests.test import TestService

from thrift.protocol.THeaderProtocol import THeaderProtocol
from thrift.transport.THeaderTransport import THeaderTransport
from thrift.transport.TSocket import TSocket
import fboss.system_tests.test.constants


class TestClient(TestService.Client):
    DEFAULT_PORT = fboss.system_tests.test.constants.DEFAULT_PORT

    def __init__(self, host, port=None, timeout=10.0):
        self.host = host
        if port is None:
            port = self.DEFAULT_PORT

        self._socket = TSocket(host, port)
        self._socket.setTimeout(timeout * 1000)
        self._transport = THeaderTransport(self._socket)
        self._protocol = THeaderProtocol(self._transport)

        self._transport.open()
        TestService.Client.__init__(self, self._protocol)

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self._transport.close()


def main(ip):
    print("in client main")
    client = TestClient(socket.gethostname())
    response = client.ping(ip)
    client._socket.close()
    print(response)


if __name__ == "__main__":
    ip = sys.argv[1]
    main(ip)
