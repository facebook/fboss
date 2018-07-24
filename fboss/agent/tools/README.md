FBOSS Python Tools
=========================

Requirements:

  apt-get install python-ipaddr python-thrift thrift-compiler
  export FBOSS=/path/to/base/of/code

Generate the python bindings from the thrift files:

  cd $FBOSS/fboss/agent/if
  for p in *.thrift; do
    thrift -I $FBOSS -r --gen py $p
    echo $p done
  done


######
# Apply a couple of fixups (fixups or ???)

Dodge the "can't find generator 'cpp2' error"

 sed -i -e 's/^namespace cpp2 facebook.fboss/#namespace cpp2 facebook.fboss/' \
    $FBOSS/fboss/agent/if/*.thrift
fboss.agent doesn't exist, but fboss.ctrl does

  sed -i -e 's/^from fboss.agent/from fboss.ctrl/' \
    $FBOSS/fboss/agent/tools/fboss_route.py


Now run the fboss_route command:

  export FBOSS_IF=$FBOSS/fboss/agent/if/gen-py
  PYTHONPATH=$FBOSS_IF/neteng/:$FBOSS/external/fbthrift/thrift/lib/py:$FBOSS_IF/:$FBOSS/external/fbthrift/thrift/lib/py         python fboss_route.py


For example:

  export FBOSS_IF=$FBOSS/fboss/agent/if/gen-py
  export PYTHONPATH=$FBOSS_IF/neteng/:$FBOSS/external/fbthrift/thrift/lib/py:$FBOSS_IF/:$FBOSS/external/fbthrift/thrift/lib/py
  python fboss_route.py host add 172.31.0.0/24 172.16.1.1
