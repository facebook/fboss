#!/usr/bin/env python3

from fboss.system_tests.testutils.delayed_signals import DelayedSignalManager
import time


class DownAllPorts(DelayedSignalManager):
    '''
    Bring all of the ports down as a context manager and bring them
    back up after we __exit__().  Build on top of DelayedSignalManager so
    that if we get a SIGTERM or similar in the middle, the ports still
    get brought back up on exit from the test.
    '''

    def __init__(self, test, retry=3):
        self.test = test
        self.downed_ports = []
        self.retry = retry
        super(DownAllPorts, self).__init__(test)

    def __enter__(self):
        # disable signal handling before downing the ports
        super(DownAllPorts, self).__enter__()
        self.down_all_port_state()

    def __exit__(self, *args):
        self.restore_all_ports_state()
        # reenable signal handling after re-enabling the ports
        super(DownAllPorts, self).__exit__(*args)

    def down_all_port_state(self):
        # get a list of ports and set their admin state to $enable
        # iff the oper state is up
        with self.test.test_topology.switch_thrift() as switch_thrift:
            ports = switch_thrift.getAllPortInfo()
            for port, port_state in ports.items():
                if port_state.operState:
                    self.test.log.info("Downing port %s  -- id=%d" %
                                        (port_state.name, port))
                    self.downed_ports.append(port)
                    self._set_port_admin_state(port, switch_thrift, False)

    def _set_port_admin_state(self, port, switch_thrift, enabled):
        switch_thrift.setPortState(port, enabled)
        dir = "Up" if enabled else "Down"
        # wait for the port to comeback up
        # don't use FB internal retryable library because this is OSS
        for i in range(self.retry):
            state = switch_thrift.getPortInfo(port)
            if state.operState == enabled:
                break
            self.test.log.info("#%d: Waiting for 1 second for port %d to come %s"
                                % (i, port, dir))
            time.sleep(1)
        if i >= self.retry:
            self.test.fail("Failed to bring port %d back %s" % (port, dir))

    def restore_all_ports_state(self):
        with self.test.test_topology.switch_thrift() as switch_thrift:
            for port in self.downed_ports:
                self.test.log.info("Restoring downed port to up : id=%d" % port)
                self._set_port_admin_state(port, switch_thrift, True)
