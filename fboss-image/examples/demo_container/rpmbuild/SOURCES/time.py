#!/usr/bin/env python3
import cowsay
import time

while True:
    time.sleep(1)

    s = cowsay.get_output_string("trex", "Current time is " + time.ctime()) + "\n"
    with open("/output/time.txt", "w") as f:
        f.write(s)
