from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import random


def get_random_test_ipv6():
    r = ""
    for i in range(0, 32):
        r += "{0:x}".format(random.randint(0, 15))
        if ((i + 1) % 4) == 0 and i != 31:
            r += ":"
    return r


def get_random_test_ipv4():
    r = ""
    for i in range(0, 4):
        r += str(random.randint(0, 255))
        if i != 3:
            r += '.'
    return r
