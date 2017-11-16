from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

from fboss.netlink_manager.netlink_manager_service import NetlinkManagerService
from thrift.protocol.THeaderProtocol import THeaderProtocol
from thrift.transport.THeaderTransport import THeaderTransport
from thrift.transport.TSocket import TSocket


class NetlinkManagerClient(NetlinkManagerService.Client):
    DEFAULT_PORT = 5912

    def __init__(self, host, port=None, timeout=5.0):
        self.host = host
        if port is None:
            port = self.DEFAULT_PORT

        self._socket = TSocket(host, port)
        self._socket.setTimeout(timeout * 1000)
        self._transport = THeaderTransport(self._socket)
        self._protocol = THeaderProtocol(self._transport)

        self._transport.open()
        NetlinkManagerService.Client.__init__(self, self._protocol)

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self._transport.close()
