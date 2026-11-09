#!/bin/bash
# LUT-hijack step 0 dump helper. Run: lut_dump.sh <label> [camera_ip]
# Dumps a physical RAM region twice-fast with address recorded in filename.
LABEL=${1:?label required}
IP=${2:-${A5100_IP:-192.168.68.104}}
ADDR=${ADDR:-0xC0000000}   # region to scan (from kimi strategy)
COUNT=${COUNT:-0x200000}   # 2 MiB window, covers more than kimi's 0x98000
cd "$(dirname "$0")/.."   # repo root
python3 cam_cmd.py IP=$IP "dd if=/dev/mem of=/tmp/lut_${LABEL}.bin bs=4 skip=\$(( ${ADDR} / 4 )) count=\$(( ${COUNT} / 4 ))" 60
python3 cam_cmd.py IP=$IP "ls -la /tmp/lut_${LABEL}.bin" 15
python3 cam_cmd.py IP=$IP "busybox ftpput -u cam -p cam -P 2121 ${A5100_MAC_IP:-192.168.68.113} lut_${LABEL}.bin /tmp/lut_${LABEL}.bin" 60
ls -la ftp-root/lut_${LABEL}.bin
