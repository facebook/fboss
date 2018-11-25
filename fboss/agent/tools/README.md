# FBOSS Python Tools

__Prerequisites__

FBOSS and its external dependencies are supposed to be compiled in order to build FBOSS Python Tools. See [facebook/fboss/build.md](https://github.com/facebook/fboss/blob/master/BUILD.md)

__Install dependencies__

```
apt install -y python-ipaddr python-pip
pip install future futures
```

__Install Facebook thrift python lib__

```
export FBOSS=/opt/fboss
cd $FBOSS/external/fbthrift/thrift/lib/py/
pip install .
```

__Generate the python bindings from the thrift files__

```
alias thrift="$FBOSS/external/fbthrift/build/bin/thrift1"

for d in $FBOSS/fboss/agent/if $FBOSS/fboss/qsfp_service/if ; do
  cd $d
  for p in *.thrift; do
    thrift -I $FBOSS -r --gen py $p
    echo "$p done"
  done
done
```

__Set Python library path__

```
export PYTHONPATH=$FBOSS/fboss/agent/if/gen-py/neteng:$FBOSS/fboss/qsfp_service/if/gen-py:$FBOSS/external/fbthrift/thrift/lib/py:$FBOSS/fboss/agent/if/gen-py
```

__Run the fboss_route command__

For example

```
python $FBOSS/fboss/agent/tools/fboss_route.py host add 172.31.0.0/24 172.16.1.1
```
