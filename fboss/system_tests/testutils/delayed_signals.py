#!/usr/bin/env python3

import os
import signal

DEFAULT_SIGNALS = [signal.SIGHUP, signal.SIGINT, signal.SIGTERM]


class DelayedSignalManager(object):
        """ Trap signals, save their handlers, and process them on __exit__
            This is super important if we really, really want to clean up
            before we exit, even if we get a SIGTERM, for example.
        """

        def __init__(self, test, signals=None):
            """
            @param test an instance of FbossBaseSystemTest
            @param signals : a list of signals as enums to handle
            """
            self.test = test
            self.signals = signals
            if self.signals is None:
                self.signals = DEFAULT_SIGNALS
            self.deferred = []
            self.handlers = {}

        def handle_signal(self, sig, stack):
            self.test.log.info("Got signal %s -- delaying" % str(sig))
            self.deferred.append(sig)

        def __enter__(self):
            # Trap each signal for handling later
            for sig in self.signals:
                self.test.log.info("Delaying handling of signal %s" % str(sig))
                self.handlers[sig] = signal.signal(sig, self.handle_signal)
            return self

        def __exit__(self, *args):
            # Restore handlers
            for sig, handler in self.handlers.items():
                if handler is None:
                    handler = signal.SIG_IGN  # wacky python
                self.test.log.info("Restoring handler for sig %s" % str(sig))
                signal.signal(sig, handler)
            # resend delayed signals
            for sig in self.deferred:
                os.kill(os.getpid(), sig)
