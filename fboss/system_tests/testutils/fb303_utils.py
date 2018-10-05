#!/usr/bin/env python3

from libfb.py.decorators import retryable
from thrift.transport.TTransport import TTransportException
from fboss.thrift_clients import FbossAgentClient


@retryable(num_tries=3,
           sleep_time=1,
           retryable_exs=[TTransportException])
def get_fb303_counter(switch_thrift: FbossAgentClient,
                      counter: str):
    """
    @param switch_thrift: FbossAgentClient object that is part of test topology
    @param counter: a string that identifies fb303 counter

    getCounter will raise an exception on failure. For now we only retry on the
    list of known exceptions above. If any other exception happens, we die
    """
    return switch_thrift.getCounter(counter)
