#!/bin/sh
# Merge bootloader, partition table and app into one image (ESP32-S3: bootloader at 0x0).
cd build && esptool.py --chip esp32s3 merge_bin -o merged-flash.bin @flash_args
