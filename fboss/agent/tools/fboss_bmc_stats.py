#!/usr/bin/python

"""
Sample script to get stats from BMC rest interface.

Assumes that the usb0 interface on BMC is 192.168.0.1 and usb0 interface
on microserver is 192.168.0.2.
"""

import pprint
import urllib2
import json
from optparse import OptionParser

apis = [
  '/api/sys',
  '/api/sys/mb',
  '/api/sys/mb/fruid',
  '/api/sys/bmc',
  '/api/sys/server',
  '/api/sys/sensors',
  '/api/sys/gpios',
  '/api/sys/modbus_registers',
  '/api/sys/psu_update',
  '/api/sys/slotid',
]

pp = pprint.PrettyPrinter()

def get_api(host, api):
  print("=== %s ===" % api)
  url = "http://%s:8080%s" % (host, api)
  d = urllib2.urlopen(url).read()
  j = json.loads(d)
  pp.pprint(j)

def get_all_apis(host):
  for api in apis:
    get_api(host, api)

def list_apis():
  for api in apis:
    print api
  print 'all'


parser = OptionParser()
parser.add_option('-l', '--list', dest='mode', action='store_const',
  const='list', help="list APIs")
parser.add_option('-g', '--get', dest='mode', action='store_const',
  const='get', help='get API')
parser.add_option('-H', '--hostname', dest='hostname', action='store',
  default='192.168.0.1', help='host/ip of BMC on usb0')

(options, args) = parser.parse_args()

if options.mode == 'list':
  list_apis()
elif options.mode == 'get':
  if args[0] == 'all':
    get_all_apis(options.hostname)
  else:
    get_api(options.hostname, args[0])
else:
  print "what?"
