#!/usr/bin/env bash

# FBOSS is now built using fbcode_builder. Thus, just invoke fbcode_builder
# getdeps.py to build fboss here.
for getdeps in build/fbcode_builder/getdeps.py opensource/fbcode_builder/getdeps.py ; do
    if [ -f $getdeps ] ; then
        exec env PATH=/opt/rh/devtoolset-8/root/usr/bin/:$PATH $getdeps build fboss
    fi
done
echo "Could not find getdeps.py!?"
exit 1
