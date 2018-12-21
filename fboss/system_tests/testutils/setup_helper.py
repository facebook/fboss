#!/usr/bin/env python3

from libfb.py.decorators import retryable
from fboss.system_tests.system_tests import FbossBaseSystemTest
from neteng.fboss.ctrl.ttypes import SwitchRunState


@retryable(num_tries=90, sleep_time=1)
def check_switch_configured(test: FbossBaseSystemTest):
    with test.test_topology.switch_thrift() as switch_thrift:
        current_state = switch_thrift.getSwitchRunState()
        # Any state after CONFIGURED is also considered to be configured
        test.assertTrue(
            current_state == SwitchRunState.CONFIGURED or
            current_state == SwitchRunState.FIB_SYNCED
        )


LOW_PRI_QUEUE = 0
MID_PRI_QUEUE = 2
HIGH_PRI_QUEUE = 9


def retry_old_counter_names(func):
    def wrapper(*args, **kwargs):
        try:
            func(*args, **kwargs)
        except Exception as e:
            obj = args[0]
            obj.cpu_low_pri_queue_prefix = ("cpu.queue%d" % LOW_PRI_QUEUE)
            obj.cpu_mid_pri_queue_prefix = ("cpu.queue%d" % MID_PRI_QUEUE)
            obj.cpu_high_pri_queue_prefix = ("cpu.queue%d" % HIGH_PRI_QUEUE)
            func(*args, **kwargs)
    return wrapper
