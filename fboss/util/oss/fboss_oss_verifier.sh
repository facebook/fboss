#!/bin/bash

sudo docker exec FBOSS_CENTOS8 /var/FBOSS/fboss_oss_verifier.py
date >> /tmp/fboss_oss_verifier_logs
