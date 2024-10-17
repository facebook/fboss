#!/bin/bash
if [[ "$2" == "no" ]]; then
    $1 --version | grep "Package Version"
fi
