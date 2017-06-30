from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
from libfb.py.controller.base import BaseController
from fboss.system_tests.test import TestService
from openr.cli.transition_tmp.breeze_cli_utils import ip_addr_to_str

import os


class TestServer(BaseController, TestService.Iface):
    SERVICE = TestService

    def ping(self, ip):
        str_ip = ip_addr_to_str(ip)
        print(str_ip)
        response = os.system("ping -c 1 " + str_ip)
        if response == 0:
            return True
        else:
            return False

    def status(self):
        return True


if __name__ == '__main__':
    TestServer.initFromCLI()
