#!/usr/bin/env python3

from logging import Logger
from libfb.py.decorators import retryable
from thrift.transport.TTransport import TTransportException
from fboss.thrift_clients import FbossAgentClient


@retryable(num_tries=5,
           sleep_time=5,
           retryable_exs=[TTransportException])
def get_fb303_counter(switch_thrift: FbossAgentClient,
                      counter: str,
                      log: Logger,
                      try_number: int):
    """
    @param switch_thrift: FbossAgentClient object that is part of test topology
    @param counter: a string that identifies fb303 counter
    @param logger: Logger object
    @param try_number: int passed in by retryable to identified current try

    getCounter will raise an exception on failure. For now we only retry on the
    list of known exceptions above. If any other exception happens, we die
    """
    log.info(
        f"Trying to get counter {counter}, try #{try_number}"
    )
    return switch_thrift.getCounter(counter)
