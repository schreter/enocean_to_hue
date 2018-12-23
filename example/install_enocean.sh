#!/bin/bash
# Example script for installing/updating the gateway
PATH=$PATH:/sbin
/etc/init.d/enocean_to_hue_gw stop
cp enocean_to_hue /usr/bin
cp example.conf /etc/enocean_to_hue.conf
/etc/init.d/enocean_to_hue_gw start

