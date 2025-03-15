#!/bin/bash

# Copies test files to target switch so pytest can discover them

#TODO:
# - execute test remotely, report back success or failure

prog="$0"

usage() {
    echo "Usage: $prog <HOSTNAME>"
    echo
    echo
    echo "Description:"
    echo "  Copies bsp_test test files to target switch"
    echo
}

POSITIONAL_ARGS=()

get_opts() {
    if [ $# -lt 1 ]; then
        usage
        exit 1;
    fi
    for param in "$@"; do
        case "$param" in
            -h|--help)
                usage
                exit;
                ;;
            -*)
                echo "Unknown option: $param"
                usage
                exit 1;
                ;;
            *)
            POSITIONAL_ARGS+=("$1")
            shift
            ;;
        esac
    done
}

get_opts "$@"
if [ ${#POSITIONAL_ARGS[@]} -lt 1 ]; then
    echo "no hostname provided"
    exit 1;
fi
hostname=${POSITIONAL_ARGS[0]}

rsync -vzr "$HOME"/fbsource/fbcode/fboss/platform/bsp_tests "root@$hostname:/tmp/"

rsync -vzr "$HOME"/fbsource/fbcode/fboss/platform/configs "root@$hostname:/tmp/"
